#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tshell.h"
#include <unistd.h>

void tshell_loop(void)
{
	char *cmd_line;
	parse_info *info;
	//struct command_type *com;
	char cwd[4096];
	int status = 0;

	do
	{
		getcwd(cwd, 4096);
		printf("tshell@%s%% ", cwd);
		cmd_line = tshell_read_line();
		if (cmd_line == NULL)
		{
			fprintf(stderr, "unable to read command\n");
			continue;
		}
	
		info = tshell_parse(cmd_line);
		if (info == NULL)
		{
			free(cmd_line);
			continue;
		}
		print_info(info);	

		status = tshell_execute(info);

		free_info(info);
		free(cmd_line);	
	} while (1);
}

int main(int argc, char **agrv)
{
	// load config files if any
	
	//run command loop
	tshell_loop();

	//perform any shutdown/cleanup

	return EXIT_SUCCESS;
}
