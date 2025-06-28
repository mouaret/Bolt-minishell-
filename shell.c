#include "shell.h"

/**
 * Initialize shell context
 */
shell_context_t* init_shell_context(void) {
    shell_context_t *ctx = malloc(sizeof(shell_context_t));
    if (!ctx) {
        perror("malloc");
        return NULL;
    }
    
    ctx->environ = environ;
    ctx->last_exit_status = 0;
    
    if (getcwd(ctx->current_dir, sizeof(ctx->current_dir)) == NULL) {
        perror("getcwd");
        strcpy(ctx->current_dir, "/");
    }
    
    return ctx;
}

/**
 * Cleanup shell context
 */
void cleanup_shell_context(shell_context_t *ctx) {
    if (ctx) {
        free(ctx);
    }
}

/**
 * Print shell prompt
 */
void print_prompt(void) {
    printf("$ ");
    fflush(stdout);
}

/**
 * Signal handler for SIGINT (Ctrl+C)
 */
void sigint_handler(int sig) {
    printf("\n");
    print_prompt();
}

/**
 * Setup signal handlers
 */
void handle_signals(void) {
    signal(SIGINT, sigint_handler);
    signal(SIGQUIT, SIG_IGN);
}

/**
 * Simple command line parser (placeholder)
 * This is where your partner's parser will be integrated
 */
command_chain_t* parse_command_line(const char *line) {
    command_chain_t *chain = create_command_chain();
    if (!chain) return NULL;
    
    // Simple tokenization for demonstration
    // Your partner's parser will replace this
    char *line_copy = strdup(line);
    char *token = strtok(line_copy, " \t\n");
    
    if (token) {
        command_node_t *node = create_command_node();
        if (node) {
            node->command = strdup(token);
            
            // Collect arguments
            char *args[MAX_ARGS];
            int argc = 0;
            
            while ((token = strtok(NULL, " \t\n")) && argc < MAX_ARGS - 1) {
                args[argc++] = token;
            }
            
            if (argc > 0) {
                node->args = copy_args(args, argc);
                node->argc = argc;
            }
            
            add_command_to_chain(chain, node);
        }
    }
    
    free(line_copy);
    return chain;
}

/**
 * Main shell loop
 */
int main(void) {
    shell_context_t *ctx = init_shell_context();
    if (!ctx) {
        fprintf(stderr, "Failed to initialize shell\n");
        return 1;
    }
    
    handle_signals();
    
    printf("Mini Shell v1.0 - POSIX Compatible\n");
    printf("Type 'exit' to quit\n\n");
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while (1) {
        print_prompt();
        
        read = getline(&line, &len, stdin);
        if (read == -1) {
            if (feof(stdin)) {
                printf("\n");
                break;
            }
            perror("getline");
            continue;
        }
        
        // Remove trailing newline
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        // Parse and execute command
        command_chain_t *chain = parse_command_line(line);
        if (chain) {
            execute_command_chain(chain, ctx);
            free_command_chain(chain);
        }
    }
    
    free(line);
    cleanup_shell_context(ctx);
    return 0;
}