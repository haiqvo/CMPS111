/*
 * Custom Shell
 * Author : Erik Swedberg, Hai Vo, and Justin Yeo
 * A basic shell written in C 
 * Able to handle &,<,>,|
 * */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

extern char **get_line();
extern char **environ;
char* id;
int sys_nerr;
int errno;

typedef void (*sighandler_t)(int);
sighandler_t old_sig;


bool back_flag;
char* re_out;//">"
char* re_in;//"<"
int pipe_count;//the store the index of "|"

void init_flags(void)//initialize all the flags
{
	back_flag = 0;
	re_out = NULL;
	re_in = NULL;
	pipe_count = -1;
}

void type_prompt(char* id)
{
   	printf("%s# ", id);
}

void kchild(int sig)//Kill the child process
{
    	if(back_flag == 0)return;
		pid_t pid;
    	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    	{
        	back_flag--;
    	}
    	if(back_flag == 0) signal(SIGCHLD, old_sig);
}

void cd(char* dir)//the cd
{
	int er = chdir(dir);
	if(er == -1) perror("chdir");
}

char** parse(char** args)//breaks the commands up and checks for special char
{
	if (args[0] == NULL) return NULL;
	if (!strcmp("exit", args[0])) exit(0);//the exit
	if(!strcmp(args[0], "cd"))// the "cd"
	{
		cd(args[1]);
        	return;
	}
	char** ret = malloc(sizeof(char*)*10);
	int i;
	int j = 0;
	for(i = 0; args[i] != NULL; i++)
	{
		char* s = args[i];
		switch(s[0])
			{
				case '&':
					back_flag++;
					old_sig = signal(SIGCHLD, kchild);
					break;
				case '>':
					if(args[++i] != NULL) re_out = strdup(args[i]);//else error
					else fprintf(stderr, "Output file path not found.\n");
					break;
				case '<':
					if(args[++i] != NULL) re_in = strdup(args[i]);//else error
					else fprintf(stderr, "Input file path not found.\n");
					break;
				case '|':
					pipe_count = i;
					break;
				default:
					ret[j++] = strdup(args[i]);
					break;
			}
	}
	ret[j+1] = NULL;//puts NULL for the execvp
	return ret;
}

char* strip_path(char* path)
{
	int i;
	char* slash = NULL;
	for(i = 0; path[i] != 0; i++)
	{
		if(path[i] == '/') slash = (path + i);
	}
	if(slash != NULL) return strdup(slash + 1);
}

void try_standard()//return stream back to user input and output
{
	if(re_out != NULL) 
	{
		re_out = NULL;
		freopen("/dev/tty", "a", stdout) == NULL;
	}
	if(re_in != NULL) 
	{
		re_in = NULL;
		freopen("/dev/tty", "r", stdin);
	}
}

void set_standard()//set stream to the command ">" or "<"
{
	if(re_out != NULL) freopen(re_out, "w", stdout);
	if(re_in != NULL) freopen(re_in, "r", stdin);
}

char* extract_path(char* PATH, int n)//gets the path 
{
    int i;
    for(i = 0; PATH[i] != '\0'; i++)
    {
        if(PATH[i] == ':')
        {
            if(!n--) break;
        }
    }
    return strndup(PATH + i, i+1);
}

void execute(char** args)//handle the execvp and checks for the binary files
{
	set_standard();
	int er = execvp(args[0], args);//run command if fail goes and checks path
    	char* PATH = getenv("PATH");
    	int limit = strlen(PATH);
    	int count = 0;//makes it so that it goes down all paths
    	char* dir = extract_path(PATH, count);
    	while(er == -1)
    	{
        	char* dir = extract_path(PATH, count);
        	char* bin = malloc(strlen(dir) + 1 + strlen(args[0]));
		strcpy(bin, dir);
        	strcat(bin, "/");
		strcat(bin, args[0]);//adds path to command
		args[0] = bin;
		execvp(args[0], args);
        	count++;
        	if(count > limit) break;
    	}
    	perror("execvp");
	exit(1);
}


void execute_pipe(char** args)//the function call when piping
{
	char** args1 = malloc(sizeof(char*)*10);
	char** args2 = malloc(sizeof(char*)*10);
	int i;
	for(i = 0; i<pipe_count; i++)
	{
		args1[i] = args[i];//splits the args into 2 args
	}
	int j;
	for(i = pipe_count, j = 0; args[i]!=NULL; i++, j++)
	{
		args2[j] = args[i];
	}
	args1[i] = NULL;
	args2[j+1] = NULL;
	int fd[2];// 0 = read, 1 = write
	pipe(fd);
	pid_t pid = fork();

	switch(pid)
	{
		case 0://child
			close(fd[0]);
			dup2(fd[1], fileno(stdout));
			close(fd[1]);
			execute(args1);//the first command
			break;
		case -1://error
    		printf("fork 1 failed\n");
    		exit(1);
    	default:
		wait(NULL);
    		close(fd[1]);
    		pid = fork();
    		switch(pid)
    		{
    			case 0:
    				dup2(fd[0], fileno(stdin));
				close(fd[0]);
    				execute(args2);//the second command
    				break;
    			case -1://error
    				printf("fork 2 failed\n");
    				exit(1);
    			default:
				wait(NULL);
    				break;
    		}
    }
}

int main(int argc, char** argv)//the main function
{	
	int status;
	id = strip_path(argv[0]);
	init_flags();
	int i;
	char **args;
	while(1)//keeps shell running until "exit" is called
	{
		type_prompt(id);
		args = get_line();//gets the command
		if(args == NULL) continue;//if blank do nothing
    		args = parse(args);//break up the command
		if(pipe_count >= 0)//handle the execute differently is pipe
		{
			execute_pipe(args);
			pipe_count = 0;
			continue;
		}
    		pid_t pid = fork();
    		switch(pid)
    		{
    			case 0://child
    				execute(args);
    				exit(1);
    				break;
    			case -1://error
    				printf("fork failed\n");
    				exit(1);
    			default:
    				if(!back_flag) 
    				{
    					wait(&status);
    				}
    		}
  	}
}
