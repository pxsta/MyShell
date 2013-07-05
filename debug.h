#if !defined _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#define SIGINTDEBUG 0
#define DEBUG 0
#define LOG_LEVEL 5

#define p_warn(level,...) if(level>=LOG_LEVEL){fprintf(stderr,__VA_ARGS__);fprintf(stderr, " Error\n");};
#define p_debug(...) if(DEBUG){fprintf(stderr,__VA_ARGS__);fprintf(stderr, "\n");}
#endif
