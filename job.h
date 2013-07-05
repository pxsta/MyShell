#if !defined _JOB_H_
#define _JOB_H_

#include <termios.h>

typedef struct process
{
    struct process *next;       /* パイプの次のプロセス */
    char **argv;                /* execvpの引数(argv[0]はコマンド名、argv[1]以降は引数) */
    pid_t pid;                  /* プロセスID */
    char completed;             /* プロセスが完了しているかどうか */
    char stopped;               /* プロセスが停止中かどうか */
    int status;                 /* waitの結果 */
    int is_builtin;             /* ビルトインコマンドかどうか */
} process;

typedef struct job
{
    struct job *next;           /* 次のジョブ */
    char *name;                 /* 表示用の名前 */
    process *first_process;     /* ジョブのプロセスリスト */
    pid_t pgid;                 /* PGID */
    char notified;              /* ジョブの状態変化をコンソールに表示済みかどうか */
    struct termios tmodes;      /* ターミナルの設定 */
    int in;                     /* 標準入力の変更先 */
    int out;                    /* 標準出力の変更先 */
    int out_err;                /* エラー出力先 */
    int foreground;
} job;

void init_shell ();
job* get_job_list_head();
void launch_job (job *j);
void request_job_notification (void);
int job_is_completed (job *j);
int job_is_stopped (job *j);
void put_job_in_foreground (job *j, int cont);
void put_job_in_background (job *j, int cont);
void continue_job (job *j, int foreground,int cont);

job* create_raw_job_node();
process* create_raw_process_node();
char* generate_job_name(job *j);
#endif
