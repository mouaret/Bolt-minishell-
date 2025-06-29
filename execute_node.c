#include "shell.h"

/**
 * Execute nodes chained of same type (of operator)
 */

int execute_piped_commands(cmd_node_t *cmd1, cmd_node_t *cmd2, shell_context_t *ctx) 
{
    if (!cmd1 || !cmd2) {
        return 1; // Invalid command nodes
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return 1;
    }

    if (pid1 == 0) { // Child process for cmd1
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipe_fd[0]); // Close unused read end
        close(pipe_fd[1]); // Close write end after dup
        return execute_node(cmd1, ctx); // Execute first command
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return 1;
    }

    if (pid2 == 0) { // Child process for cmd2
        dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin from pipe
        close(pipe_fd[0]); // Close read end after dup
        close(pipe_fd[1]); // Close unused write end
        return execute_node(cmd2, ctx); // Execute second command
    }

    close(pipe_fd[0]); // Parent closes both ends of the pipe
    close(pipe_fd[1]);

    int status;
    waitpid(pid1, &status, 0); // Wait for first command to finish
    waitpid(pid2, &status, 0); // Wait for second command to finish

    return WEXITSTATUS(status);
}   


int execute_node(cmd_node_t *node, shell_context_t *ctx) 
{
    if (!node || !node->command) {
        return 1; // No command to execute
    }

    // Check if it's a built-in command
    if (is_builtin_command(node->command)) {
        return execute_builtin_command(node, ctx);
    }

    // Handle piped commands
    if (node->type == CMD_PIPE && node->next) {
        return execute_piped_commands(node, node->next, ctx);
    }

    // Execute external command
    return execute_external_command(node, ctx);
}


