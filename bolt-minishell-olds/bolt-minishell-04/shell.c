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
    (void)sig; /* Suppress unused parameter warning */
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
 * Parse a single token, handling quotes
 */
char* parse_token(const char **input) {
    const char *start = *input;
    const char *current = *input;
    char quote_char = 0;
    int escaped = 0;
    
    /* Skip leading whitespace */
    while (*current && (*current == ' ' || *current == '\t')) {
        current++;
    }
    
    if (!*current) {
        *input = current;
        return NULL;
    }
    
    start = current;
    
    /* Handle quoted strings */
    if (*current == '"' || *current == '\'') {
        quote_char = *current;
        current++; /* Skip opening quote */
        start = current; /* Start after the quote */
        
        while (*current && (*current != quote_char || escaped)) {
            if (*current == '\\' && !escaped) {
                escaped = 1;
            } else {
                escaped = 0;
            }
            current++;
        }
        
        if (*current == quote_char) {
            /* Found closing quote */
            int len = current - start;
            char *token = malloc(len + 1);
            if (!token) {
                perror("malloc");
                return NULL;
            }
            strncpy(token, start, len);
            token[len] = '\0';
            current++; /* Skip closing quote */
            *input = current;
            return token;
        } else {
            /* Unclosed quote - treat as regular token */
            current = start - 1; /* Go back to include the quote */
        }
    }
    
    /* Handle regular tokens */
    start = current;
    while (*current && *current != ' ' && *current != '\t' && *current != '\n') {
        current++;
    }
    
    if (current == start) {
        *input = current;
        return NULL;
    }
    
    int len = current - start;
    char *token = malloc(len + 1);
    if (!token) {
        perror("malloc");
        return NULL;
    }
    strncpy(token, start, len);
    token[len] = '\0';
    
    *input = current;
    return token;
}

/**
 * Simple command line parser (placeholder)
 * This is where your partner's parser will be integrated
 */
command_chain_t* parse_command_line(const char *line) {
    command_chain_t *chain = create_command_chain();
    if (!chain) return NULL;
    
    const char *current = line;
    char *token = parse_token(&current);
    
    if (token) {
        cmd_node_t *node = create_cmd_node();
        if (!node) {
            free(token);
            free_command_chain(chain);
            return NULL;
        }
        
        node->command = token;
        
        /* Collect arguments */
        char *args[MAX_ARGS];
        int argc = 0;
        
        while ((token = parse_token(&current)) && argc < MAX_ARGS - 1) {
            args[argc++] = token;
        }
        
        if (argc > 0) {
            node->args = malloc((argc + 1) * sizeof(char*));
            if (!node->args) {
                perror("malloc");
                /* Clean up collected arguments */
                for (int i = 0; i < argc; i++) {
                    free(args[i]);
                }
                free_cmd_node(node);
                free_command_chain(chain);
                return NULL;
            }
            
            for (int i = 0; i < argc; i++) {
                node->args[i] = args[i];
            }
            node->args[argc] = NULL;
            node->argc = argc;
        }
        
        add_command_to_chain(chain, node);
    }
    
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
        
        /* Remove trailing newline */
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        
        /* Skip empty lines */
        if (strlen(line) == 0) {
            continue;
        }
        
        /* Parse and execute command */
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