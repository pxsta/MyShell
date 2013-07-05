#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "builtin.h"
#include "job.h"
#include "debug.h"

typedef enum builtin_command_types
{
    JOBS = 1,
    FG,
    BG,
    EXIT,
} builtin_command_types;

typedef struct builtin_command
{
    char *name;
    builtin_command_types command_type;
} builtin_command;

const builtin_command builtin_comands[] =
{
    {"jobs", JOBS},
    {"fg", FG},
    {"bg", BG},
    {"exit", EXIT},
};

#define BUILTIN_COUNT (sizeof builtin_comands / sizeof *builtin_comands)

builtin_command_types get_command_type(const char *command)
{
    size_t i;
    for (i = 0; i < BUILTIN_COUNT; i++)
    {
        if (strcmp(command, builtin_comands[i].name) == 0)
        {
            return builtin_comands[i].command_type;
        }
    }
    return -1;
}


int builtin_exists(const char *command)
{
    int res = get_command_type(command);
    return res > 0;
}


int builtin_run(const char *command, const char **argv)
{
    builtin_command_types command_type = get_command_type(command);
    switch (command_type)
    {
    case JOBS:
    {
        int i;
        job *j;
        job *job_list = get_job_list_head();
        for (i = 0, j = job_list; j; i++, j = j->next)
        {
            if (j->pgid != getpgrp())
            {
                if (job_is_completed(j))
                {
                    fprintf(stdout, "[%d]: End\t%s(%ld)\n", i, j->name, (long)j->pgid);
                }
                else if (job_is_stopped(j))
                {
                    fprintf(stdout, "[%d]: Stopped\t%s(%ld)\n", i, j->name, (long)j->pgid);
                }
                else
                {
                    fprintf(stdout, "[%d]: Running%s\t%s(%ld)\n", i, j->foreground ? "" : "(background)", j->name, (long)j->pgid);
                }
            }
        }
        return 0;
    }
    case BG:
    {
        int i;
        job *job_list = get_job_list_head();
        job *j = job_list;

        int index = 0;
        if (argv[1] != NULL)
        {
            index = atoi(argv[1]);
        }

        for (i = 0; i < index;)
        {
            if (j->pgid != getpgrp())
            {
                i++;
            }
            j = j->next;

            if (j == NULL)
            {
                return -1;
            }
        }
        p_debug("bg %s(%ld)\n", j->name, (long)j->pgid);
        j->foreground = 0;
        continue_job(j, 0, job_is_stopped(j));
        return 0;
    }
    case FG:
    {
        int i;
        job *job_list = get_job_list_head();
        job *j = job_list;

        int index = 0;
        if (argv[1] != NULL)
        {
            index = atoi(argv[1]);
        }

        for (i = 0; i < index;)
        {
            if (j->pgid != getpgrp())
            {
                i++;
            }
            j = j->next;
            if (j == NULL)
            {
                return -1;
            }
        }
        p_debug("fg %s(%ld)\n", j->name, (long)j->pgid);
        j->foreground = 1;
        continue_job(j, 1, job_is_stopped(j));
        return 0;
    }
    case EXIT:
    {
        int i;
        job *j;
        job *job_list = get_job_list_head();
        for (i = 0, j = job_list; j; i++, j = j->next)
        {
            if (j->pgid != getpgrp())
            {
                killpg(j->pgid, 2);
            }
        }
        //ここは親プロセスなのでそのまま終了してしまう
        exit(0);
    }
    default:
        fprintf(stderr, "builtin-command not found:%s\n", command);
        break;
    }
    return -1;
}
