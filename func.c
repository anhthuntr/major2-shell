#include "major2.h"


// ---------------------
// Built-in Command
// ---------------------

char *builtin_cmd[]={"cd", "path", "myhistory", "exit"};

char* getHomeDir() {
	uid_t uid = getuid();
	struct passwd* pwd = getpwuid(uid);
	if (!pwd) {
		fprintf(stderr, "User with %u ID is unknown.\n", uid);
		exit(EXIT_FAILURE);
	}

	return pwd->pw_dir;
}

//change current working directory
//if no argument is passed, it will change to user's HOME dicrectory
int builtin_cd(char **args) {
	char cwd[256];
	if (args[1] == NULL) {
		char* homeDir = getHomeDir();
		if (chdir(homeDir)==EXIT_FAILURE)
			return EXIT_FAILURE;
		printf("change directory to %s\n", homeDir);
	} else {
		if (chdir(args[1]) != 0)
			perror("major2: cd");
		else if (chdir(args[1])==0)
		{
			getcwd(cwd, sizeof(cwd)); //get current working directory
			printf("change directory to %s\n", cwd);
		}
	}
	return 1;
}

//check number of pipes, no redirection allowed, up two 2 pipes and 3 cmds
//return number of pipes
int pipeCheck(char *args)
{
	int i=0;
	int counter=0;

	while (args[i]!='\0')
	{
		if (args[i] == '|')
			counter++;
		else if ((args[i] == '>' || args[i] == '<') && counter > 0) //no redirection
			return -1;
		i++;
	}
	return counter;
}

// ---------------------
// Parsing Input
// ---------------------

//to read a single line input from command
char* readLine() {
/*
	char *input = NULL;
	size_t sz = 0;
	ssize_t line = getline(&input, &sz, stdin);
	if (line < 0) {
		if (FALSE) {
			perror("error");
		}
		exit(1);
	}
	return input;
*/
  int bufsize = BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;
  if (!buffer) {
    fprintf(stderr, "allocation error\n");
    exit(EXIT_FAILURE);
  }
  while (1) {
    c = getchar();
    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    if (position >= bufsize) {
      bufsize += BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  return buffer;
}

//to split a line and tokenize a string
char** splitLine(char *line) {
	int i=0; //iteration variable
	int bufsize=32;

	char **tokens=malloc(bufsize*sizeof(char*));
	if (!tokens) {
		perror("allocation error\n");
		exit(1);
	}

	char *delimiters = " \t\r\n\a";
	char *token = strtok(line, delimiters);

	while (token!= NULL) {
		tokens[i]=token;
		i++;

		//if exceeded the buffer, reallocate
		if (i >= bufsize) {
			bufsize += 32;
			tokens=realloc(tokens,bufsize*sizeof(char*));
			
			//allocation failed
			if (!tokens) {
				perror("allocation error\n");
				exit(1);
			}
		}

		token = strtok(NULL, delimiters);
	
}

	tokens[i]=NULL;
	return tokens;
}

//split arguments with provided delimiter
char** semiSplit(char *input, const char *dem) {
	int position=0;
	size_t bufsize=0;
	char *token;
	char **tokens = malloc(bufsize*sizeof(char*));

	if (!tokens) {
		fprintf(stderr, "allocation error\n");
		exit(EXIT_FAILURE);
	}
	token = strtok(input, dem);
	while (token != NULL) {
		tokens[position]=token;
		position++;
		token=strtok(NULL,dem);
	}
	tokens[position]=NULL;
	return tokens;
}

// ---------------------
// Executing command
// ---------------------

int process(char **args) {
	pid_t pid = fork();
	int stat_loc;
	if (pid == 0) {
		//child error
		if (execvp(args[0], args) == -1 ) {
			perror("Couldn't run the command");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		//forking error
		perror("Fork error");
	} else {
		do {
			waitpid(pid, &stat_loc, WUNTRACED);
		} while (!WIFEXITED(stat_loc) && !WIFSIGNALED(stat_loc));
	}

	return 1;
}

//normal exec
int exec(char **args) {
	if (args[0]==NULL)
		return 1;

	//match built-in command with user-input from command-line arguments
	for (int i=0;i<(sizeof(builtin_cmd)/sizeof(char*));i++) {
		if(strcmp(args[0],builtin_cmd[i])==0) {
			switch(i) {
			case 0: return (builtin_cd(args)); break;
			case 1: printf("this is path command\n"); break;
			case 2: printf("this is myhistory command\n"); break;
			case 3: printf("exit\n"); break;
			}
		}
	}

	return process(args);
}

//execute commands with pipe, support up to 3 commands
void execPipe(char** buf,int num){
	int fd[3][2],i;
	char **argv;

	for(i=0;i<num;i++){
		//after read from input with provided delimeters
		//tokenize the string
		argv=splitLine(buf[i]);

		if(i!=num-1){
			if(pipe(fd[i])<0){
				perror("failed creating pipes\n");
				return;
			}
		}
		//children
		if(fork()==0){
			if(i!=num-1){
				dup2(fd[i][1],1);
				close(fd[i][0]);
				close(fd[i][1]);
			}

			if(i!=0){
				dup2(fd[i-1][0],0);
				close(fd[i-1][1]);
				close(fd[i-1][0]);
			}
			execvp(argv[0],argv);
			perror("invalid input ");
			exit(1);//in case exec is not successfull, exit
		}
		//parent
		if(i!=0){//second process
			close(fd[i-1][0]);
			close(fd[i-1][1]);
		}
		wait(NULL);
	}
}

// ---------------------
// Execution Mode
// ---------------------

//interactive mode
void interactive() {
	char **args, **line;
	char *input;
	int counter;

	while (TRUE) {
		printf("major2> ");
		input=readLine();
		args=semiSplit(input, ";");
		for (int i=0; args[i]!=NULL;++i) {
			if (strcmp(args[i]," ")!= 0) {
				counter=pipeCheck(args[i]); //get number of pipes

				if (counter==0) { //no pipe
					line = splitLine(args[i]);
					exec(line);
					free(line);
				}
				else if (counter > 0 && counter < 3) { //1 or 2 pipe
					//printf("acceptable pipe found\n");
					//split arguments with |
					char **args2=semiSplit(input, "|");

					//counter only return number of pipe
					//param here is number of commands
					execPipe(args2,counter+1);
					free(args2);
					
				}
				else { //have redirection or more than 2 pipes
					printf("invalid pipe\n");
				}
			}

		}
		free(args);
	}
}

//batch mode
void batch(char file[100]) {
	printf("File \"%s\" opening...", file);
	FILE *fptr;
	char input[500];
	char **buf, **args;
	int counter;

	fptr = fopen(file, "r");
	if (fptr == NULL)
	{
		fprintf(stderr, "Error: File Not Found\n");
		exit(1);
	} else {
		printf("\nFile Opened.\n");
		while (fgets(input, sizeof(input), fptr)!=NULL) {
			printf("\n%s", input); //print each command in file batch
			args=semiSplit(input,";");
			for (int i=0; args[i]!=NULL; ++i) {
				counter=pipeCheck(args[i]);
				if (counter==0) {
					buf=splitLine(args[i]);
					exec(buf);
					free(buf);
				}
				else if (counter>0 && counter<3) {
					char **args2=semiSplit(input,"|");
					execPipe(args2, counter+1);
					free(args2);
				} else {
					printf("invalid pipe\n");
				}
			}
		}
	}
	free(args);
	fclose(fptr);
}