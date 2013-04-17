#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

extern char **get_line();
extern char **environ;

struct condition_flags
{
	bool background;// &
	FILE* out;// >
	FILE* in;// <
	bool pipe;// |
};
typedef struct condition_flags *flags;

flags init_flags(void)
{
	flags ret = malloc(sizeof(struct condition_flags));
	ret->background = 0;
	ret->out = NULL;
	ret->in = NULL;
	ret->pipe = 0;
}

char** parse(flags f, char** args)
{
	if (!strcmp("exit", args[0])) exit(0);
	else if(!strcmp("cd", args[0]))
	{
		int er = chdir(args[1]);
		return NULL;
	}
	char** ret = malloc(sizeof(char*)*10);
	ret[0] = "NULL";
	int i;
	int j = 0;
	for(i = 0; args[i] != NULL; i++)
	{
		char* s = args[i];
		switch(s[0])
			{
				case '&':
					f->background = 1;
					break;
				case '>':
					if(args[++i] != NULL) f->out = freopen(args[i], "w", stdout);//else error
					break;
				case '<':
					if(args[++i] != NULL) f->in = freopen(args[i], "r", stdin);//else error
					break;
				case '|':
					f->pipe = 1;
					break;
				default:
					ret[j++] = strdup(args[i]);
					break;
			}
	}
	ret[j+1] = NULL;
	return ret;
}

void type_prompt(char* id)
{
   printf("%s# ", id);
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

void execute(char** args, flags f)
{
            int er = execvp(args[0], args);
            if(er == -1)
            {
                    char *exe = malloc(strlen("/bin/") + strlen(args[0]));
                    strcpy(exe, "/bin/");
                    strcat(exe, args[0]);
                    args[0] = exe;
                    execvp(args[0], args);
            }
}

int main(int argc, char** argv) 
{
	int status;
	char* id = strip_path(argv[0]);
	flags f = init_flags();
	int i;
	char **args;

	while(1) 
	{
		type_prompt(id);
		args = get_line();
    	args = parse(f, args);
    	if(args == NULL) continue;
    	if(fork() != 0)
    	{
			waitpid(-1, &status, 0);
			if(f->out != NULL) fclose(f->out);
			if(f->in != NULL) fclose(f->in);
		}
    	else
    	{
    		execute(args, f);
    		exit(1);
    	}
  	}
}
