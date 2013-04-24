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

typedef void (*sighandler_t)(int);
sighandler_t old_sig;

struct condition_flags
{
	bool background;// &
	char* out;// >
	char* in;// <
	bool pipe;// |
};
typedef struct condition_flags *flags;
flags f;

flags init_flags(void)
{
	flags ret = malloc(sizeof(struct condition_flags));
	ret->background = 0;
	ret->out = NULL;
	ret->in = NULL;
	ret->pipe = -1;
}

void type_prompt(char* id)
{
   printf("%s# ", id);
}

void kchild(int sig)
{
    if(f->background == 0)return;
	pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        kill(pid, SIGKILL);//, SIGTERM);
        f->background--;
    }
        if(f->background == 0)
            signal(SIGCHLD, old_sig);
        
        //type_prompt(id);
}

char** parse(char** args)
{
	if (!strcmp("exit", args[0])) exit(0);
	char** ret = malloc(sizeof(char*)*10);
	int i;
	int j = 0;
	for(i = 0; args[i] != NULL; i++)
	{
		char* s = args[i];
		switch(s[0])
			{
				case '&':
					f->background++;
					old_sig = signal(SIGCHLD, kchild);
					break;
				case '>':
					if(args[++i] != NULL) f->out = strdup(args[i]);//freopen(args[i], "w", stdout);//else error
					break;
				case '<':
					if(args[++i] != NULL) f->in = strdup(args[i]);//freopen(args[i], "r", stdin);//else error
					break;
				case '|':
					f->pipe = i;
					break;
				default:
					ret[j++] = strdup(args[i]);
					break;
			}
	}
	ret[j+1] = NULL;
        if(!strcmp(args[0], "cd"))
	{
		chdir(args[1]);
                return NULL;
	}
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

void try_stdout()
{
	if(f->out != NULL) 
	{
		f->out = NULL;
		freopen("/dev/tty", "a", stdout);
	}
}

void try_stdin()
{
	if(f->in != NULL) 
	{
		f->in = NULL;
		freopen("/dev/tty", "r", stdin);
	}
}

void try_standard()
{
	try_stdout();
	try_stdin();
}

void set_standard()
{
	if(f->out != NULL) freopen(f->out, "w", stdout);
	if(f->in != NULL) freopen(f->in, "r", stdin);
}

char* extract_path(char* PATH, int n)
{
    int i;
    for(i = 0; PATH[i] != NULL; i++)
    {
        if(PATH[i] == ':')
        {
            if(!n--) break;
        }
    }
    return strndup(PATH + i, i+1);
}

void execute(char** args)
{
	
	set_standard();
	int er = execvp(args[0], args);
    char* PATH = getenv("PATH");
    int limit = strlen(PATH);
    int count = 0;
    char* dir = extract_path(PATH, count);
    while(er == -1)
    {
        char* dir = extract_path(PATH, count);
        char* bin = malloc(strlen(dir) + 1 + strlen(args[0]));
		strcpy(bin, dir);
        strcat(bin, "/");
		strcat(bin, args[0]);
		args[0] = bin;
		execvp(args[0], args);
        count++;
        if(count > limit) break;
    }
	try_standard();
	printf("Executable file not found->\n");
	exit(1);
}

void excutePipe(char** args){
char** args1 = malloc(sizeof(char*)*10);
char** args2 = malloc(sizeof(char*)*10);
int i;
printf("args1:\n");
for(i = 0; i<=f->pipe; i++)
{
	args1[i] = args[i]; 
	printf("%s\n", args1[i]);
}
printf("args2:\n");
for(i = f->pipe+1; args[i]!=NULL; i++)
{
	args2[i] = args[i];
	printf("%s\n", args2[i]);
}
}

int main(int argc, char** argv) 
{
	/*char* PATH = getenv("PATH");
	printf("PATH=%s\n",PATH);*/
	
	int status;
	id = strip_path(argv[0]);
	f = init_flags();
	int i;
	char **args;

	while(1) 
	{
		type_prompt(id);
		args = get_line();
                /*
		printf("Argument list:\n");
		for(i = 0; args[i] != NULL; i++)
		{
			printf("%s\n", args[i]);
		}
                 * */
    	args = parse(args);
    	if(args == NULL) continue;
	if(f->pipe >= 0)
	{
		excutePipe(args);
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
    			printf("fork failed->\n");
    			exit(1);
    		default:
    			if(!f->background) 
    			{
    				wait(&status);
    				fprintf(stderr, "Child process \"%s\" terminated\n", args[0]);
    			}
    			//try_standard(f);
    	}
  	}
}
