#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#define TRUE 1
#define FALSE 0
#define MAX_LINE_LENGTH 512
#define BUFSIZE 512

char* gethomeDir();
int builtin_cd(char **args);
int numBuiltin();
int pipeCheck(char *args);
char* readLine();
char** splitLine(char *line);
char** semiSplit(char *input, const char *dem);
int process(char **args);
int exec(char **args);
void execPipe(char** buf,int num);
void interactive();
void batch(char file[100]);

