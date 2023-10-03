#include "major2.h"


// ---------------------
// Built-in Command
// ---------------------

char *builtin_cmd[]={"cd", "myhistory", "exit"};

//change current working directory
//if no argument is passed, it will change to user's HOME dicrectory
int builtin_cd(char **args) {
	char cwd[256];
	if (args[1] == NULL) {
		//getenv to get the path stored in the $HOME environment variable
		char* homeDir = getenv("HOME");
		if (chdir(homeDir)==EXIT_FAILURE)
			return EXIT_FAILURE;
		printf("change directory to %s\n", homeDir);
	} else {
		if (chdir(args[1]) != 0)
			perror("error");
		else if (chdir(args[1])==0)
		{
			getcwd(cwd, sizeof(cwd)); //get current working directory
			printf("change directory to %s\n", cwd);
		}
	}
	return 1;
}

void appendHistory(char ***history, char *args) {
	char *line = malloc(sizeof(char) * 512);
	if (line == NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(EXIT_FAILURE);
	}

	strncpy(line, args, 512);

	if (strchr(line, '\n') != NULL) {
		line[strlen(line) - 1] = '\0';
	}

	int i;
	for (i = 0; (*history)[i] != NULL; i++) {
	}
	
	if (i == 20) { //if history is full, remove the oldest command
		for (i = 0; i < 19; i++) {
			(*history)[i] = (*history)[i + 1];
		}
		(*history)[19] = line;
	} else {
		(*history)[i] = line;
	}
}

int builtin_history(char **args, char ***history) {
	if (args[1] == NULL) {
		for (int i = 0; (*history)[i] != NULL; i++) {
			printf("%s\n", (*history)[i]);
		}
	} else if (strcmp(args[1], "-c") == 0) {
		for (int i = 0; i < 20; i++) {
			(*history)[i] = NULL;
		}
	} else if (strcmp(args[1], "-e") == 0) {
		if (args[2] == NULL) {
			printf("error: no history number specified\n");
		} else {
			int num = atoi(args[2]);

			if (num > 20 || num < 0) {
				printf("error: history number out of range\n");
			} else {
				char *cmd = (*history)[num];

				if (cmd == NULL) {
					printf("error: history number out of range\n");
				} else {
					printf("%s\n", cmd);
					appendHistory(history, cmd);
					exec(splitLine(cmd), history);
				}
			}
		}
	} else {
		printf("error: invalid argument %s\n", args[1]);
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

	char *input = NULL;
	size_t sz = MAX_LINE_LENGTH;
	ssize_t line = getline(&input, &sz, stdin);
	if (line < 0) {
		if (FALSE) {
			perror("error");
		}
		exit(1);
	}
	return input;
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
int exec(char **args, char ***history) {
	if (args[0]==NULL)
		return 1;

	int returnValue = -2;
	int counter=0;
	char *input = malloc(MAX_LINE_LENGTH*sizeof(char*));
	char buf[MAX_LINE_LENGTH]="";
	bool pipe=false;

	int stdin = dup(0); //saves stdin
	int stdout = dup(1); //saves stdout

	//check for redirection
	int j=0;
	int in=0;
	int out=0;
	while (args[j]!=NULL) {
		if (strcmp(args[j], "<") == 0) {
			in = j;
		} else if (strcmp(args[j], ">") == 0) {
			out = j;
		} else if (strcmp(args[j], "|") == 0) {
			pipe=true;
		}
		j++;
	}

	//pipe detect
	if (pipe) {
		for (int i=0; args[i]!=NULL;++i) {
			if (strcmp(args[i]," ")!= 0) {
				strncpy(buf,args[i],MAX_LINE_LENGTH);
				strcat(buf," ");
				strncat(input, buf, MAX_LINE_LENGTH);
				counter=pipeCheck(input);
			}
		}
		
		if (counter>0 && counter<3) {
			char **args2 = semiSplit(input,"|");
			execPipe(args2,counter+1);
			free(args2);
		}
		else if (counter==-1)
			printf("no redirection allowed.\n");
		else 
			fprintf(stderr, "only 2 pipes are allowed.\n");
	}

	//if redirection
	if (in > 0) {
		int fd = open(args[in+1], O_RDONLY);
		if (fd < 0) {
			perror("error");
			return 1;
		}
		dup2(fd, 0);
		close(fd);
		args[in] = NULL;
	}

	if (out > 0) {
		int fd = open(args[out+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
		if (fd < 0) {
			perror("error");
			return 1;
		}
		dup2(fd, 1);
		close(fd);
		args[out] = NULL;
	}

	//match built-in command with user-input from command-line arguments
	for (int i=0;i<(sizeof(builtin_cmd)/sizeof(char*));i++) {
		if(strcmp(args[0],builtin_cmd[i])==0) {
			switch(i) {
			case 0: returnValue = (builtin_cd(args)); break;
			case 1: returnValue = (builtin_history(args, history)); break;
			case 2: printf("exit\n"); returnValue = -1; break;
				break;
			}
		}
	}

	if (returnValue == -2 && !pipe) {
		returnValue = process(args);
	}

	dup2(stdin, 0); //restore stdin
	dup2(stdout, 1); //restore stdout

	//close old stdin and stdout
	close(stdin);
	close(stdout);

	return returnValue;
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
void interactive(char prompt[]) {
	char **args, **line, **history;
	char *input;
	bool end=false;
	
	regex_t whiteSpace;
	if (regcomp(&whiteSpace, "^[[:space:]]*$", 0)) {
		fprintf(stderr, "Regex failed to compile\n");
		exit(1);
	}

	history = malloc(20*sizeof(char*));
	if (!history) {
		fprintf(stderr, "Could not allocate memory for history\n");
		exit(1);
	}

	while (!end) 
	{
		printf("%s> ", prompt);
		input=readLine();
		args=semiSplit(input, ";");
		
		for (int i=0; args[i]!=NULL;++i) 
		{
			if (regexec(&whiteSpace, args[i], 0, NULL, 0) == REG_NOMATCH) 
			{
				if (strcmp(args[i], "exit")==0) 
				{
					end = true;
					continue;
				}

				appendHistory(&history, args[i]);

				line = splitLine(args[i]);
				if (exec(line, &history)==-1) end=true;
				free(line);
				
			}
		}

		if (end==true)
			//builtin exit goes here
		free(args);

	} //end while-loop

	free(history);
	regfree(&whiteSpace);
	exit(0);
}

//batch mode
void batch(char file[100]) {
	FILE *fptr;
	char input[512];
	char **buf, **args, **history;
	int counter;
	bool end = false;
	
	regex_t whiteSpace;
	if (regcomp(&whiteSpace, "^[[:space:]]*$", 0)) {
		fprintf(stderr, "Regex failed to compile\n");
		exit(1);
	}

	history = malloc(20*sizeof(char*));
	if (!history) {
		fprintf(stderr, "Could not allocate memory for history\n");
		exit(1);
	}

	fptr = fopen(file, "r");
	if (fptr == NULL)
	{
		fprintf(stderr, "Error: File Not Found\n");
		printf("exiting...bye bye!\n");
		exit(1);
	} 

	else 
	{
		printf("File \"%s\" opening...", file);
		while (!end && fgets(input, sizeof(input), fptr) != NULL) 
		{
			printf("\n%s", input); //print each command in file batch
			args=semiSplit(input,";");
			for (int i=0; args[i]!=NULL; ++i) 
			{
				if (strcmp(args[i], "exit")==0)
				{
					end = true;
					continue;
				}

				if (regexec(&whiteSpace, args[i], 0, NULL, 0) == REG_NOMATCH) {
					appendHistory(&history, args[i]);

					buf=splitLine(args[i]);
					if(exec(buf, &history)==-1) //builtin exit goes here
					free(buf);
				}
			}
		}
	}

	free(args);
	free(history);
	regfree(&whiteSpace);
	fclose(fptr);
	exit(0);
}
