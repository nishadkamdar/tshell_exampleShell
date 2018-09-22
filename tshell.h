#define PIPE_MAX_NUM 10
#define FILE_MAX_SIZE 40

struct command_history {
	char cmd_and_var_args[100];
	int cmd_num;
};

struct command_type {
	char **cmd_and_var_args;
	int var_num;
};

typedef struct {
	int bool_infile;
	int bool_outfile;
	int bool_background;
	struct command_type comm_array[PIPE_MAX_NUM];
	int pipe_num;
	char in_file[FILE_MAX_SIZE];
	char out_file[FILE_MAX_SIZE];
} parse_info;

parse_info *tshell_parse(char *);
int tshell_execute(parse_info *);
void free_info(parse_info *);
void print_info(parse_info *);
char *tshell_read_line(void);
parse_info *tshell_parse(char *cmdline);
int tshell_jobs(parse_info *p_info);
int tshell_help(parse_info *p_info);
int tshell_cd(parse_info *p_info);
int tshell_exit(parse_info *p_info);
int tshell_kill(parse_info *p_info);
int tshell_history(parse_info *p_info);
void update_history(char *cmd);

void *xcalloc(int nr_elements, int size_per_element);
