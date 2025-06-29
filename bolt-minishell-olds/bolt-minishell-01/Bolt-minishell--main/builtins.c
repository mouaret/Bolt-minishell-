#include "shell.h"

/**
 * Built-in cd command
 */
int builtin_cd(char **args, shell_context_t *ctx) {
    char *dir;
    
    if (!args || !args[0]) {
        // No argument, go to HOME
        dir = getenv("HOME");
        if (!dir) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    } else {
        dir = args[0];
    }
    
    if (chdir(dir) != 0) {
        perror("cd");
        return 1;
    }
    
    // Update current directory in context
    if (getcwd(ctx->current_dir, sizeof(ctx->current_dir)) == NULL) {
        perror("getcwd");
        return 1;
    }
    
    return 0;
}

/**
 * Built-in pwd command
 */
int builtin_pwd(char **args, shell_context_t *ctx) {
    char cwd[MAX_PATH];
    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd");
        return 1;
    }
}

/**
 * Built-in echo command
 */
int builtin_echo(char **args, shell_context_t *ctx) {
    int newline = 1;
    int start = 0;
    
    // Check for -n flag
    if (args && args[0] && strcmp(args[0], "-n") == 0) {
        newline = 0;
        start = 1;
    }
    
    // Print arguments
    if (args) {
        for (int i = start; args[i]; i++) {
            if (i > start) {
                printf(" ");
            }
            printf("%s", args[i]);
        }
    }
    
    if (newline) {
        printf("\n");
    }
    
    return 0;
}

/**
 * Built-in env command
 */
int builtin_env(char **args, shell_context_t *ctx) {
    extern char **environ;
    
    if (!environ) {
        return 1;
    }
    
    for (char **env = environ; *env; env++) {
        printf("%s\n", *env);
    }
    
    return 0;
}

/**
 * Built-in exit command
 */
int builtin_exit(char **args, shell_context_t *ctx) {
    int exit_code = ctx->last_exit_status;
    
    if (args && args[0]) {
        exit_code = atoi(args[0]);
    }
    
    printf("exit\n");
    cleanup_shell_context(ctx);
    exit(exit_code);
}