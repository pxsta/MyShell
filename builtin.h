#if !defined _BUILTIN_H_
#define _BUILTIN_H_

int builtin_exists(const char* command);
int builtin_run(const char* command,const char** argv);

#endif
