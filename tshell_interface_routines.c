#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tshell.h"
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <proc/readproc.h>
#include <errno.h>

#define TSHELL_RL_BUFSIZE 1024

#define TSHELL_TOK_BUFSIZE 64
#define TSHELL_TOK_DELIM " \t\r\n\a"

static int index_t; 
struct command_history cmd_history[10];
int procs[4194303];

char *builtin_str[] = {
	"jobs",
	"cd",
	"help",
	"exit",
	"kill",
	"history"
};

int (*builtin_func[]) (parse_info *p_info) = {
	&tshell_jobs,
	&tshell_cd,
	&tshell_help,
	&tshell_exit,
	&tshell_kill,
	&tshell_history
};

int tshell_num_builtins(void)
{
	return sizeof(builtin_str) / sizeof(char *);
}

int tshell_jobs(parse_info *p_info)
{
	int i = 1;
	memset(procs, 0, 4194303 * sizeof(int));
	PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS);

	proc_t proc_info;
	memset(&proc_info, 0, sizeof(proc_info));
	while(readproc(proc, &proc_info) != NULL)
	{
		procs[i] = proc_info.tid;	
		printf("%d:%20s:\t%5ld\t%5d\n",
		       i, proc_info.cmd, proc_info.resident,
		       proc_info.tid);
		i++;
	}

	closeproc(proc);
}

int tshell_cd(parse_info *p_info)
{
	if(p_info->comm_array[0].cmd_and_var_args[1] == NULL)
	{
		printf("tshell: expected argument to \"cd\"\n");
	}
	else
	{
		if(chdir(p_info->comm_array[0].cmd_and_var_args[1]) != 0)
		{
			printf("tshell: error changing directory\n");
			exit(EXIT_FAILURE);
		}
	}
	return 1;
}

int tshell_help(parse_info *p_info)
{
	int i;

	printf("tshell\n");
	printf("Type programs name and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for(i = 0; i < tshell_num_builtins(); i++)
		printf("%s\n", builtin_str[i]);

	printf("Use the man command for information on other programs\n");
	return 1;
}

int tshell_exit(parse_info *p_info)
{
	pid_t cur;
	cur = getpid();
	int status = 0, wpid = 0;

	if((wpid = waitpid(-1, &status, WNOHANG)) == 0)
	{
		int i = 1;

		printf("Backgorund processes exist\n");
		printf("please kill all background processes before exiting\n");
		memset(procs, 0, 4194303 * sizeof(int));
		PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS);

		proc_t proc_info;
		memset(&proc_info, 0, sizeof(proc_info));
		while(readproc(proc, &proc_info) != NULL)
		{
			if (proc_info.ppid == cur)	
				printf("%d:%20s:\t%5ld\t%5d\n",
			       i, proc_info.cmd, proc_info.resident,
			       proc_info.tid);
		}

		closeproc(proc);	
	}
	
	else if (wpid < 0)
	{
		exit(EXIT_SUCCESS);
	}
}

int tshell_kill(parse_info *p_info)
{
	int pid = 0;
	char *token = NULL;
	int s = 0;

	if (p_info->comm_array[0].cmd_and_var_args[1][0] == '%')
	{
		token = strtok(p_info->comm_array[0].cmd_and_var_args[1], "%");	
		pid = procs[atoi(token)];
		
		while(token != NULL)
		{
			printf("%s\n", token);
			token =	strtok(NULL, "%");
		}
	}
	else
	{
		pid = atoi(p_info->comm_array[0].cmd_and_var_args[1]);
	}

	s = kill(pid, 0);
	if (s == 0)
	{
		kill(pid, SIGKILL);
	}
	else 
	{
		if (errno == EPERM)
			printf("Permission denied\n");
		else if (errno == ESRCH)
			printf("Process does not exist\n");
		else
			exit(EXIT_FAILURE);
	}
}

int tshell_history(parse_info *p_info)
{
	int i = 0, j = 0;
	for (i = 0; i < 10; i++)
	{
		printf("%d: %s\n",cmd_history[i].cmd_num, cmd_history[i].cmd_and_var_args);
	}
}	

void update_history(char *buffer)
{
	static int i;
	static int cmd_num;
	int j = 0, k = 0;

	if (i >= 10)
		i = 0;

	memset(cmd_history[i].cmd_and_var_args, 0, 100 * sizeof(cmd_history[i].cmd_and_var_args[0]));
	strcpy(cmd_history[i].cmd_and_var_args, buffer);
	
	//printf("%s\n", cmd_history[i].cmd_and_var_args);	

	cmd_num++;
	cmd_history[i].cmd_num = cmd_num;
	
	index_t = i;
	i++;
}

char *tshell_read_line(void)
{
	int bufsize = TSHELL_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(bufsize * sizeof(char));
	int c;

	if (!buffer)
	{
		fprintf(stderr, "tshell: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		c = getchar();

		if (c == EOF || c == '\n')
		{
			buffer[position] = '\0';
			update_history(buffer);

			return buffer;
		}
		else
		{
			buffer[position] = c;
		}
		position++;

		if (position >= bufsize)
		{
			bufsize += TSHELL_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer)
			{
				fprintf(stderr, "tshell: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}

}

/*void update_history(parse_info *p_info)
{
	int cmd_len = 0;
	static int i;
	static int cmd_num;
	int j = 0, k = 0;

	cmd_len = p_info->comm_array[0].var_num;

	if (i >= 10)
		i = 0;

	for(j = 0; j < cmd_len; j++)
		strcpy(cmd_history[i].cmd_and_var_args[j], p_info->comm_array[0].cmd_and_var_args[j]);

	cmd_num++;
	cmd_history[i].cmd_num = cmd_num;

	i++;
}*/

parse_info *tshell_parse(char *cmdline)
{
	parse_info *result;
	int bufsize = TSHELL_TOK_BUFSIZE, position = 0;
	char *token;
	
	result = xcalloc(1, sizeof(parse_info));
	
	result->comm_array[0].cmd_and_var_args = malloc(bufsize * sizeof(char *));
	if (!result->comm_array[0].cmd_and_var_args)
	{
		fprintf(stderr, "tshell: allocation error\n");
		exit(EXIT_FAILURE);
	}
	
	token = strtok(cmdline, TSHELL_TOK_DELIM);
	while (token != NULL)
	{
		if (!strcmp(token, "<"))
		{
			result->bool_infile = 1;
			
			/*token = strtok(NULL, TSHELL_TOK_DELIM);
			strcpy(result->in_file, token);

			goto out;*/
		}

		if (!strcmp(token, ">"))
		{
			result->bool_outfile = 1;
			
			/*token = strtok(NULL, TSHELL_TOK_DELIM);
			strcpy(result->out_file, token);

			goto out;*/
		}

		if (!strcmp(token, "&"))
		{
			result->bool_background = 1;

			//goto out;
		}

		if (position && !strcmp(result->comm_array[0].cmd_and_var_args[position - 1], "<") && result->bool_infile)
		{
			strcpy(result->in_file, token);
		}

		if (position && !strcmp(result->comm_array[0].cmd_and_var_args[position - 1], ">") && result->bool_outfile)
		{
			strcpy(result->out_file, token);
		}
		
		result->comm_array[0].cmd_and_var_args[position] = token;
		result->comm_array[0].var_num++;
		position++;

		if (position >= bufsize)
		{
			bufsize += TSHELL_TOK_BUFSIZE;
			result->comm_array[0].cmd_and_var_args[position] = realloc(result->comm_array[0].cmd_and_var_args, bufsize * sizeof (char *));
			if (!result->comm_array[0].cmd_and_var_args)
			{
				fprintf(stderr, "tshell: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, TSHELL_TOK_DELIM);
	}

out:
	result->comm_array[0].cmd_and_var_args[position] = NULL;
	//update_history(result);
	//tshell_history(result);
	return result;
}

void free_info(parse_info *p_info)
{
	free(p_info->comm_array[0].cmd_and_var_args);
	free(p_info);
}

void print_info(parse_info *p_info)
{
	int i = 0;

	printf("arg1: %s\n", p_info->comm_array[0].cmd_and_var_args[0]);
	for (i = 0; i < (p_info->comm_array[0].var_num - 1); i++)
	{
		printf("arg%d: %s\n",i+2, p_info->comm_array[0].cmd_and_var_args[i+1]);
	}

	if(p_info->bool_infile)
	{
		printf("input redirection: yes\n");
		printf("input file: %s\n", p_info->in_file);
	}
	else
	{
		printf("input redirection: no\n");
	}
	
	if(p_info->bool_outfile)
	{
		printf("output redirection: yes\n");
		printf("output file: %s\n", p_info->out_file);
	}
	else
	{
		printf("output redirection: no\n");
	}
	
	if(p_info->bool_background)
	{
		printf("background process: yes\n");
	}
	else
	{
		printf("background process: no\n");
	}
}

int tshell_start_cmd(parse_info *p_info)
{
	char *token = NULL;
	int cmd = 0, i = 0;
	parse_info *p;

	token = strtok(p_info->comm_array[0].cmd_and_var_args[0], "!");	
	cmd = atoi(token);

	if (cmd < 0)
	{
		cmd = index_t + cmd;
		if (cmd < 0)
			cmd = 10+cmd;
		printf("cmd=%d\n", cmd);
			
		p = tshell_parse(cmd_history[cmd].cmd_and_var_args);
		tshell_execute(p);
		return 0;
	}

	for (i = 0; i < 10; i++)
	{
		if (cmd_history[i].cmd_num == cmd)
		{
			p = tshell_parse(cmd_history[i].cmd_and_var_args);
			tshell_execute(p);
			return 0;
		}	
	}

	printf("Command not found\n");
}

int tshell_execute(parse_info *p_info)
{
	pid_t pid, wpid;
	int status;
	int fd;
	char *cmd_and_var_args[10];
	int i, j = 0;

	for (j = 0; j < tshell_num_builtins(); j++)
	{
		if(strcmp(p_info->comm_array[0].cmd_and_var_args[0], builtin_str[j]) == 0)
		{
			return (*builtin_func[j])(p_info);
		}
	}

	if (p_info->comm_array[0].cmd_and_var_args[0][0] == '!')
		return tshell_start_cmd(p_info);

	pid = fork();
	if(pid == 0)
	{
		if (p_info->bool_outfile)
		{
			fd = open(p_info->out_file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

			if(fd != STDOUT_FILENO)
			{
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}

			for(i = 0; (strcmp(p_info->comm_array[0].cmd_and_var_args[i], ">") != 0); i++)
			{
				cmd_and_var_args[i] = p_info->comm_array[0].cmd_and_var_args[i];
			}
			cmd_and_var_args[i] = NULL;
			
			if (execvp(cmd_and_var_args[0], cmd_and_var_args) == -1)
			{
				printf("error executing execvp\n");
			}
			exit(EXIT_FAILURE);
	
		}
		else if (p_info->bool_infile)
		{
			fd = open(p_info->in_file, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

			if(fd != STDIN_FILENO)
			{
				dup2(fd, STDIN_FILENO);
				close(fd);
			}
			
			for(i = 0; (strcmp(p_info->comm_array[0].cmd_and_var_args[i], "<") != 0); i++)
			{
				cmd_and_var_args[i] = p_info->comm_array[0].cmd_and_var_args[i];
			}
			cmd_and_var_args[i] = NULL;
			
			if (execvp(cmd_and_var_args[0], cmd_and_var_args) == -1)
			{
				printf("error executing execvp\n");
			}
			exit(EXIT_FAILURE);
		}
		else if (p_info->bool_background)
		{
			for(i = 0; (strcmp(p_info->comm_array[0].cmd_and_var_args[i], "&") != 0); i++)
			{
				cmd_and_var_args[i] = p_info->comm_array[0].cmd_and_var_args[i];
			}
			cmd_and_var_args[i] = NULL;
			
			if (execvp(cmd_and_var_args[0], cmd_and_var_args) == -1)
			{
				printf("error executing execvp\n");
			}
			exit(EXIT_FAILURE);
		}
		else 
		{
			if (execvp(p_info->comm_array[0].cmd_and_var_args[0], p_info->comm_array[0].cmd_and_var_args) == -1)
			{
				printf("error executing execvp\n");
			}
			exit(EXIT_FAILURE);
		}
	}
	else if (pid < 0)
	{
		printf("error forking new process\n");
	}
	else
	{
		if (p_info->bool_background)
		{
			if((wpid = waitpid(pid, &status, WNOHANG)) == 0)
				return 1;
			else if (wpid < 0)
			{
				printf("failed to return from child process\n");
				exit(EXIT_FAILURE);
			}
				
		}
		else
		{
			do
			{
				wpid = waitpid(pid, &status, WUNTRACED);
			} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		}
	}

	return 1;
}
