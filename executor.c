#include "shell.h"

/**
 * Execute a chain of commands
 */
int execute_command_chain(command_chain_t *chain, shell_context_t *ctx) {
    if (!chain || !chain->head) {
        return 0;
    }
    
    command_node_t *current = chain->head;
    int last_status = 0;
    
    while (current) {
        command_node_t *next = current->next;
        
        // Handle piped commands
        if (current->type == CMD_PIPE && next) {
            last_status = execute_piped_commands(current, next, ctx);
            current = next->next; // Skip the second command in pipe
        } else {
            last_status = execute_single_command(current, ctx);
            
            // Handle conditional execution
            if (next) {
                if (current->type == CMD_AND && last_status != 0) {
                    break; // Stop on failure with &&
                }
                if (current->type == CMD_OR && last_status == 0) {
                    break; // Stop on success with ||
                }
            }
            
            current = next;
        }
        
        ctx->last_exit_status = last_status;
    }
    
    return last_status;
}

/**
 * Execute a single command
 */
int execute_single_command(command_node_t *cmd, shell_context_t *ctx) {
    if (!cmd || !cmd->command) {
        return 1;
    }
    
    // Check if it's a built-in command
    if (is_builtin_command(cmd->command)) {
        return execute_builtin_command(cmd, ctx);
    } else {
        return execute_external_command(cmd, ctx);
    }
}

/**
 * Execute built-in commands
 */
int execute_builtin_command(command_node_t *cmd, shell_context_t *ctx) {
    if (strcmp(cmd->command, "cd") == 0) {
        return builtin_cd(cmd->args, ctx);
    } else if (strcmp(cmd->command, "pwd") == 0) {
        return builtin_pwd(cmd->args, ctx);
    } else if (strcmp(cmd->command, "echo") == 0) {
        return builtin_echo(cmd->args, ctx);
    } else if (strcmp(cmd->command, "env") == 0) {
        return builtin_env(cmd->args, ctx);
    } else if (strcmp(cmd->command, "exit") == 0) {
        return builtin_exit(cmd->args, ctx);
    }
    
    return 1; // Unknown built-in
}

/**
 * Execute external commands
 */
int execute_external_command(command_node_t *cmd, shell_context_t *ctx) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        
        // Handle input redirection
        if (cmd->input_file) {
            FILE *input = fopen(cmd->input_file, "r");
            if (!input) {
                perror(cmd->input_file);
                exit(1);
            }
            dup2(fileno(input), STDIN_FILENO);
            fclose(input);
        }
        
        // Handle output redirection
        if (cmd->output_file) {
            FILE *output = fopen(cmd->output_file, 
                               cmd->append_output ? "a" : "w");
            if (!output) {
                perror(cmd->output_file);
                exit(1);
            }
            dup2(fileno(output), STDOUT_FILENO);
            fclose(output);
        }
        
        // Prepare arguments for execvp
        char **exec_args = malloc((cmd->argc + 2) * sizeof(char*));
        exec_args[0] = cmd->command;
        for (int i = 0; i < cmd->argc; i++) {
            exec_args[i + 1] = cmd->args[i];
        }
        exec_args[cmd->argc + 1] = NULL;
        
        // Execute the command
        execvp(cmd->command, exec_args);
        
        // If execvp returns, there was an error
        fprintf(stderr, "%s: command not found\n", cmd->command);
        free(exec_args);
        exit(127);
        
    } else if (pid > 0) {
        // Parent process
        if (!cmd->background) {
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        } else {
            printf("[%d] %s\n", pid, cmd->command);
            return 0;
        }
    } else {
        perror("fork");
        return 1;
    }
}

/**
 * Execute piped commands
 */
int execute_piped_commands(command_node_t *cmd1, command_node_t *cmd2, shell_context_t *ctx) {
    int pipe_fd[2];
    pid_t pid1, pid2;
    int status1, status2;
    
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return 1;
    }
    
    // First command (left side of pipe)
    pid1 = fork();
    if (pid1 == 0) {
        close(pipe_fd[0]); // Close read end
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipe_fd[1]);
        
        // Prepare and execute first command
        char **exec_args = malloc((cmd1->argc + 2) * sizeof(char*));
        exec_args[0] = cmd1->command;
        for (int i = 0; i < cmd1->argc; i++) {
            exec_args[i + 1] = cmd1->args[i];
        }
        exec_args[cmd1->argc + 1] = NULL;
        
        execvp(cmd1->command, exec_args);
        fprintf(stderr, "%s: command not found\n", cmd1->command);
        free(exec_args);
        exit(127);
    }
    
    // Second command (right side of pipe)
    pid2 = fork();
    if (pid2 == 0) {
        close(pipe_fd[1]); // Close write end
        dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin from pipe
        close(pipe_fd[0]);
        
        // Prepare and execute second command
        char **exec_args = malloc((cmd2->argc + 2) * sizeof(char*));
        exec_args[0] = cmd2->command;
        for (int i = 0; i < cmd2->argc; i++) {
            exec_args[i + 1] = cmd2->args[i];
        }
        exec_args[cmd2->argc + 1] = NULL;
        
        execvp(cmd2->command, exec_args);
        fprintf(stderr, "%s: command not found\n", cmd2->command);
        free(exec_args);
        exit(127);
    }
    
    // Parent process - close both ends of pipe and wait
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    
    // Return exit status of the last command in the pipe
    return WEXITSTATUS(status2);
}

/**
 * Check if a command is built-in
 */
int is_builtin_command(const char *command) {
    const char *builtins[] = {"cd", "pwd", "echo", "env", "exit", NULL};
    
    for (int i = 0; builtins[i]; i++) {
        if (strcmp(command, builtins[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}