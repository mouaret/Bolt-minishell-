#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define MAX_PATH 256

/* Command types for chained execution */
typedef enum {
    CMD_SIMPLE,     /* Simple command */
    CMD_AND,        /* Command with && */
    CMD_OR,         /* Command with || */
    CMD_PIPE,       /* Command with | */
    CMD_SEMICOLON   /* Command with ; */
} cmd_type_t;

/* Command node structure for chained list */
typedef struct cmd_node {
    char *command;                  /* Command name */
    char **args;                    /* Command arguments */
    int argc;                       /* Argument count */
    cmd_type_t type;               /* Command type */
    struct cmd_node *next;      /* Next command in chain */
    
    /* Redirection information */
    char *input_file;              /* Input redirection file */
    char *output_file;             /* Output redirection file */
    int append_output;             /* Append output flag */
    int background;                /* Background execution flag */
} cmd_node_t;

/* Command chain structure */
typedef struct {
    cmd_node_t *head;          /* First command in chain */
    cmd_node_t *tail;          /* Last command in chain */
    int count;                     /* Number of commands */
} command_chain_t;

/* Shell context */
typedef struct {
    char **environ;                /* Environment variables */
    int last_exit_status;         /* Last command exit status */
    char current_dir[MAX_PATH];   /* Current working directory */
} shell_context_t;

/* Function prototypes */

/* Command chain management */
command_chain_t* create_command_chain(void);
cmd_node_t* create_cmd_node(void);
void add_command_to_chain(command_chain_t *chain, cmd_node_t *node);
void free_command_chain(command_chain_t *chain);
void free_cmd_node(cmd_node_t *node);

/* Command execution */
int execute_command_chain(command_chain_t *chain, shell_context_t *ctx);
int execute_single_command(cmd_node_t *cmd, shell_context_t *ctx);
int execute_builtin_command(cmd_node_t *cmd, shell_context_t *ctx);
int execute_external_command(cmd_node_t *cmd, shell_context_t *ctx);
int execute_piped_commands(cmd_node_t *cmd1, cmd_node_t *cmd2, shell_context_t *ctx);

/* Built-in commands */
int builtin_cd(char **args, shell_context_t *ctx);
int builtin_pwd(char **args, shell_context_t *ctx);
int builtin_echo(char **args, shell_context_t *ctx);
int builtin_env(char **args, shell_context_t *ctx);
int builtin_exit(char **args, shell_context_t *ctx);

/* Utility functions */
int is_builtin_command(const char *command);
char** copy_args(char **args, int argc);
void print_prompt(void);
void handle_signals(void);

/* Shell initialization and cleanup */
shell_context_t* init_shell_context(void);
void cleanup_shell_context(shell_context_t *ctx);

#endif /* SHELL_H */