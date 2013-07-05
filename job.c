#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include "job.h"
#include "builtin.h"
#include "debug.h"

pid_t shell_pgid;
struct termios shell_tmodes;
int terminal_fd;
job *job_list_head = NULL;


void wait_for_job (job *j);
void update_status ();
void print_job_status (int index, job *j, const char *status);
void set_all_job_running (job *j);
void free_job(job *job);
void add_job_list(job *j);
void cleanup_job_list();

job *get_job_list_head()
{
    return job_list_head;
}

void add_job_list(job *j)
{
    job *current_job;
    if (j == NULL)
    {
        fprintf(stderr, "add_job_list:job is NULL\n");
    }
    if (job_list_head == NULL)
    {
        job_list_head = j;
    }
    else
    {
        current_job = job_list_head;
        while (current_job->next)
        {
            current_job = current_job->next;
        }
        current_job->next = j;
    }
}

void free_job(job *j)
{
    process *current_pro = j->first_process;
    process *temp_pro;
    p_debug("free_job:%s", current_pro->argv[0]);
    while (current_pro)
    {
        temp_pro = current_pro;
        free(current_pro);
        current_pro = temp_pro->next;
    }
    free(j);
}

//完了したジョブをジョブ管理用のリストから削除する
void cleanup_job_list()
{
    //TODO:freeが必要
    job *current_job = job_list_head;
    job *prev = NULL;
    p_debug("cleanup_job_list");
    while (current_job)
    {
        if (job_is_completed(current_job))
        {
            //先頭
            if (prev == NULL && current_job == job_list_head)
            {
                job_list_head = current_job->next;
            }
            //2つ以上あるときの最後以外
            else if (current_job->next)
            {
                //途中にあるときはそれを飛ばして繋ぎ直す
                if (prev)
                {
                    prev->next = current_job->next;
                }
            }
            //最後
            else
            {
                if (prev == NULL)
                {
                    //何もない
                }
                else
                {
                    //最後
                    prev->next = current_job;
                }
            }
        }
        prev = current_job;
        current_job = current_job->next;
    }
    p_debug("end cleanup_job_list");
}

int job_is_stopped (job *j)
{
    process *p;

    for (p = j->first_process; p; p = p->next)
    {
        if (!p->completed && !p->stopped)
        {
            return 0;
        }
    }
    p_debug("job_is_stopped:%s", j->name);
    return 1;
}

int job_is_completed (job *j)
{
    process *p;
    for (p = j->first_process; p; p = p->next)
    {
        if (p != NULL && !p->completed)
        {
            return 0;
        }
    }
    p_debug("job_is_completed:%s", j->name);
    return 1;
}

//シェルの初期化処理を行う
void init_shell ()
{
    terminal_fd = STDIN_FILENO;
    shell_pgid = getpgrp ();

    //ジョブ管理のためにシグナルを意図的に無視する
#if !SIGINTDEBUG
    signal(SIGINT, SIG_IGN);
#endif
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);


    //自身を新たな自身のプロセスグループに属させる
    shell_pgid = getpid ();
    //PGIDセット
    if (setpgid (shell_pgid, shell_pgid) < 0)
    {
        p_warn(5, "setpgid");
        exit (1);
    }

    //自身のPGIDをターミナルのフォアグラウンドプロセスにセットする
    if (ioctl(terminal_fd, TIOCSPGRP, &shell_pgid) < 0)
    {
        p_warn(5, "ioctl");
    }

    //ターミナルの設定を保存しておく
    tcgetattr (terminal_fd, &shell_tmodes);
}


void launch_process (process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground)
{
    pid_t pid;
    p_debug("launch_process(%d):%s in:%d out:%d", getpid (), p->argv[0], infile, outfile);
    //PGIDをセットする
    pid = getpid ();
    if (pgid == 0)
    {
        pgid = pid;
        p->pid = pid;
    }
    p_debug("setpgid(%d):%s in:%d out:%d pid:%d pgid:%d", getpid (), p->argv[0], infile, outfile, pid, pgid);
    if (setpgid (pid, pgid) < 0)
    {
        perror("setpgid");
    }
    if (foreground)
    {
        if (ioctl(terminal_fd, TIOCSPGRP, &pgid) < 0)
        {
            perror("ioctl");
        }
    }

    //シグナル受信時の動作をデフォルトに戻す
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);

    //標準入出力を付け替える
    if (infile != STDIN_FILENO)
    {
        p_debug("%s:infile %d=>%d\tclose(%d)", p->argv[0], STDIN_FILENO, infile, infile);
        dup2 (infile, STDIN_FILENO);
        close (infile);
    }
    if (outfile != STDOUT_FILENO)
    {
        p_debug("%s:outfile %d=>%d\tclose(%d)", p->argv[0], STDOUT_FILENO, outfile, outfile);
        dup2 (outfile, STDOUT_FILENO);
        close (outfile);
    }
    if (errfile != STDERR_FILENO)
    {
        //不用意に標準入出力を閉じないため
        if (errfile != STDIN_FILENO && errfile == STDOUT_FILENO)
        {
            p_debug("%s:errfile %d=>%d\tclose(%d)", p->argv[0], STDERR_FILENO, errfile, errfile);
            dup2 (errfile, STDERR_FILENO);
            close (errfile);
        }
    }

    //コマンド実行
    p_debug("exec:%s", p->argv[0]);
    execvp (p->argv[0], p->argv);

    perror ("execvp");
    exit (1);
}

//ジョブの起動を行う
void launch_job (job *j)
{
    process *p;
    pid_t pid;
    int mypipe[2], infile, outfile;
    char forked = 0;

    add_job_list(j);
    cleanup_job_list();

    for (; j; j = j->next)
    {
        infile = j->in;
        for (p = j->first_process; p; p = p->next)
        {
            p->is_builtin = builtin_exists(p->argv[0]);

            //標準出力の設定
            if (p->next)
            {
                //次のプロセスがあるときはパイプのためパイプで繋ぐ
                if (pipe (mypipe) < 0)
                {
                    perror ("pipe");
                    exit (1);
                }
                outfile = mypipe[1];
            }
            else
            {
                //出力先をジョブの出力先にする
                outfile = j->out;
            }


            //ビルトインコマンドと外部コマンドで分ける
            if (p->is_builtin)
            {
                int result = 0;
                int prevDiscriptor[2];

                //現在の入出力のバックアップ
                prevDiscriptor[0] = dup(0);
                prevDiscriptor[1] = dup(1);

                //ビルトインコマンドのジョブ管理関連は親プロセスで実行
                if (j->pgid == 0 && j->first_process == p)
                {
                    p->pid = getpid();
                    j->pgid = getpgrp();
                    j->notified=1;
                }

                p_debug("builtin_run:%s in:%d out:%d", p->argv[0], infile, outfile);

                //標準入出力付け替え
                if (infile != STDIN_FILENO)
                {
                    p_debug("%s:infile %d=>%d\tclose(%d)", p->argv[0], STDIN_FILENO, infile, infile);
                    dup2 (infile, STDIN_FILENO);
                    close (infile);
                }
                if (outfile != STDOUT_FILENO)
                {
                    p_debug("%s:outfile %d=>%d\tclose(%d)", p->argv[0], STDOUT_FILENO, outfile, outfile);
                    dup2 (outfile, STDOUT_FILENO);
                    close (outfile);
                }
                if (j->out_err != STDERR_FILENO)
                {
                    //不用意に標準入出力を閉じないため
                    if (j->out_err != STDIN_FILENO && j->out_err != STDOUT_FILENO)
                    {
                        p_debug("%s:errfile %d=>%d\tclose(%d)", p->argv[0], STDERR_FILENO, j->out_err, j->out_err);
                        dup2 (j->out_err, STDERR_FILENO);
                        close (j->out_err);
                    }
                }

                //ビルトインコマンド実行
                result = builtin_run(p->argv[0], (const char **)p->argv);
                p->completed = 1;
                if (result != 0)
                {
                    fprintf(stderr, "builtin-command %s failed:%d\n", p->argv[0], result);
                }

                //標準入出力を元に戻す
                if (infile != STDIN_FILENO)
                {
                    dup2(prevDiscriptor[0], STDIN_FILENO);
                    close(prevDiscriptor[0]);
                }
                if (outfile != STDOUT_FILENO)
                {
                    dup2(prevDiscriptor[1], STDOUT_FILENO);
                    close(prevDiscriptor[1]);
                }
            }
            //外部コマンド
            else
            {
                p_debug("fork:%s in:%d out:%d", p->argv[0], infile, outfile);
                forked = 1;
                pid = fork ();
                if (pid == 0)
                {
                    p_debug("launched_job(parent):%s(%d) in:%d out:%d", p->argv[0], pid, infile, outfile);

                    //プロセス起動
                    launch_process (p, j->pgid, infile, outfile, j->out_err, j->foreground);
                    p_debug("after_launch_process");
                }
                else if (pid < 0)
                {
                    perror ("fork");
                    exit (1);
                }
                else
                {
                    p_debug("launched_job(child):%s(%d) in:%d out:%d", p->argv[0], pid, infile, outfile);
                    //親と子でどちらが先に処理が進むのかわからないため両方でPGIDをセットしておく
                    p->pid = pid;
                    if (!j->pgid)
                    {
                        j->pgid = pid;
                    }
                    p_debug("setpgid(parent)(%d):%s in:%d out:%d pid:%d pgid:%d", getpid (), p->argv[0], infile, outfile, pid, j->pgid);
                    if (setpgid (pid, j->pgid) < 0)
                    {
                        perror("setpgid");
                    }
                }
            }

            //パイプを閉じる＆繋ぐ
            if (infile != j->in)
            {
                close (infile);
            }
            if (outfile != j->out)
            {
                close (outfile);
            }
            infile = mypipe[0];
        }

        if (forked)
        {
            if (j->foreground)
            {
                put_job_in_foreground (j, 0);
            }
            else
            {
                put_job_in_background (j, 0);
            }
        }
    }

}

//ジョブをフォアグラウンドプロセスとする
void put_job_in_foreground (job *j, int cont)
{
    p_debug("put_job_in_foreground:%d",  j->pgid);

    //新規ジョブのPGIDを端末のフォアグラウンドプロセスにセットする
    if (ioctl(terminal_fd, TIOCSPGRP, &(j->pgid)) < 0)
    {
        perror("ioctl");
    }

    //プロセスが停止中の場合にはSIGCONTを送る
    if (cont)
    {
        tcsetattr (terminal_fd, TCSADRAIN, &j->tmodes);
        if (kill (-j->pgid, SIGCONT) < 0)
        {
            perror ("kill (SIGCONT)");
        }
    }

    //プロセスが完了するまでブロックする
    wait_for_job (j);


    //シェルを端末のフォアグラウンドプロセスに戻す
    if (ioctl(terminal_fd, TIOCSPGRP, &shell_pgid) < 0)
    {
        perror("ioctl");
    }

    //ターミナルの設定を戻す
    tcgetattr (terminal_fd, &j->tmodes);
    tcsetattr (terminal_fd, TCSADRAIN, &shell_tmodes);
}

//ジョブをバックグラウンドプロセスとする
void put_job_in_background (job *j, int cont)
{
    p_debug("put_job_in_background:%d",  j->pgid);
    //プロセスが停止中の場合にはSIGCONTを送る
    if (cont)
    {
        if (kill (-j->pgid, SIGCONT) < 0)
        {
            perror ("kill (SIGCONT)");
        }
    }
}

//指定したpidのプロセスのstatusをセットするためのリスト操作関数
int mark_process_status (pid_t pid, int status)
{
    job *j;
    process *p;
    p_debug("mark_process_status pid:%ld status:%d", (long)pid, status);
    if (pid > 0)
    {
        for (j = job_list_head; j; j = j->next)
        {
            for (p = j->first_process; p; p = p->next)
            {
                if (p->pid == pid)
                {
                    p->status = status;
                    if (WIFSTOPPED (status))
                    {
                        p->stopped = 1;
                    }
                    else
                    {
                        p->completed = 1;
                        if (WIFSIGNALED (status))
                        {
                            fprintf (stderr, "%ld: Terminated with signal:%d\n", (long) pid, WTERMSIG (p->status));
                        }
                    }
                    return 0;
                }
            }
        }
        p_warn(3, "No child process(pid:%ld)", (long)pid);
        return -1;
    }
    else if (pid == 0 || errno == ECHILD)
    {
        //対象の子プロセスが居ない
        return -1;
    }
    else
    {
        if (ECHILD == errno)
        {
        }
        else if (EINTR == errno)
        {
        }
        else
        {
            //wait自体が失敗
        }
        p_warn(3, "waitpid");
        return -1;
    }
}

//プロセスの状態を確認する
void update_status ()
{
    p_debug("update_status");
    int status;
    pid_t pid;

    do
    {
        //WNOHANGでブロックせずに確認＆回収
        pid = wait3 (&status, WUNTRACED | WNOHANG, 0);
        if (pid == -1)
        {
            p_warn(3, "wait3");
        }
    }
    while (!mark_process_status (pid, status));
}

//ジョブの全てのプロセスが完了するまで待機する
void wait_for_job (job *j)
{
    int status;
    pid_t pid;
    p_debug("wait_for_job:%s(pgid:%ld pid:%ld)", *j->first_process->argv, (long)j->pgid, (long)j->first_process->pid);
    do
    {
        //ジョブのプロセスグループのみ監視
        pid = wait4 (-j->pgid, &status, WUNTRACED, 0);
        if (pid == -1)
        {
            p_warn(3, "wait4");
        }
    }
    while (!mark_process_status (pid, status) && !job_is_stopped (j) && !job_is_completed (j));
}


//ジョブの状態を標準出力に表示
void print_job_status (int index, job *j, const char *status)
{
    fprintf(stdout, "[%d]\t%s\t%s (pgid:%ld)\n", index, status, j->name, (long)j->pgid);
}

//ジョブの情報が更新されていたら表示するように要求する
void request_job_notification (void)
{
    int i;
    job *j, *prev_job, *next_job;

    //プロセスの状態を確認
    update_status ();

    prev_job = NULL;
    for (i = 0, j = job_list_head; j; i++, j = next_job)
    {
        next_job = j->next;
        //ジョブの全ての子プロセスが完了したかどうか
        if (job_is_completed (j))
        {
            if (!j->foreground&&!j->notified)
            {
                j->notified = 1;
                print_job_status (i, j, "Completed");
            }

            //標準入出力が変更されていたら閉じる
            if (j->in > 2)
            {
                if (close(j->in) < 0)
                {
                    p_warn(3, "close");
                }
            }
            if (j->out > 2)
            {
                if (close(j->out) < 0)
                {
                    p_warn(3, "close");
                }
            }

            //ジョブ管理から完了したジョブを削除
            cleanup_job_list();
        }
        //ジョブが停止中かどうか
        else if (job_is_stopped (j) && !j->notified)
        {
            if (!j->foreground)
            {
                print_job_status (i, j, "Stopped");
                j->notified = 1;
                prev_job = j;
            }
        }
        //ジョブが実行中の時
        else
        {
            prev_job = j;
        }
    }
}

//全てのジョブのstopフラグを折って実行中にする
void set_all_job_running (job *j)
{
    process *p;
    for (p = j->first_process; p; p = p->next)
    {
        p->stopped = 0;
    }
    j->notified = 0;
}

//ジョブをフォアグラウンドまたはバックグラウンドに移動する
void continue_job (job *j, int foreground, int cont)
{
    set_all_job_running (j);
    if (foreground)
    {
        put_job_in_foreground (j, cont);
    }
    else
    {
        put_job_in_background (j, cont);
    }
}



job *create_raw_job_node()
{
    job *new_job_node = (job *)calloc(1, sizeof(job));
    new_job_node->next = NULL;
    new_job_node->pgid = 0;
    new_job_node->notified = 0;
    new_job_node->name = NULL;
    new_job_node->first_process = NULL;
    new_job_node->in = 0;
    new_job_node->out = 1;
    new_job_node->foreground = 1;
    return new_job_node;
}

process *create_raw_process_node()
{
    process *new_process_node = (process *)calloc(1, sizeof(process));
    new_process_node->argv = NULL;
    new_process_node->next = NULL;
    new_process_node->completed = 0;
    new_process_node->is_builtin = 0;
    return new_process_node;
}

char *generate_job_name(job *j)
{
    char *result = NULL;
    int strLength = 0;
    process *current_process = NULL;
    char **temp_argv = NULL;
    int i, k;
    for (i = 0, current_process = j->first_process; current_process; i++, current_process = current_process->next)
    {
        temp_argv = current_process->argv;
        if (temp_argv == NULL)
        {
            break;
        }
        if (i > 0)
        {
            //パイプの分とスペースの分の2を足す
            strLength += 2;
        }
        for (k = 0; temp_argv[k]; k++)
        {
            strLength += strlen(temp_argv[k]);
        }
        //スペースの文を足す
        strLength += k;
    }
    if (!j->foreground)
    {
        strLength += 2;
    }

    result = (char *)malloc(sizeof(char) * strLength + 1);
    for (i = 0, current_process = j->first_process; current_process; i++, current_process = current_process->next)
    {
        temp_argv = current_process->argv;
        if (temp_argv == NULL)
        {
            break;
        }
        if (i > 0)
        {
            sprintf(result, "%s %s", result, "|");
        }
        for (k = 0; temp_argv[k]; k++)
        {
            sprintf(result, "%s %s", result, temp_argv[k]);
        }
    }
    if (!j->foreground)
    {
        sprintf(result, "%s %s", result, "&");
    }
    return result;
}
