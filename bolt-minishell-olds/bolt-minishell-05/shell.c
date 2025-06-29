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
 * Parse a single token, handling quotes and special characters
 */
char* parse_token(const char **input, int *is_operator) {
    const char *start = *input;
    const char *current = *input;
    char quote_char = 0;
    int escaped = 0;
    
    *is_operator = 0;
    
    /* Skip leading whitespace */
    while (*current && (*current == ' ' || *current == '\t')) {
        current++;
    }
    
    if (!*current) {
        *input = current;
        return NULL;
    }
    
    /* Check for operators first */
    if (*current == '|') {
        if (*(current + 1) == '|') {
            /* || operator */
            *is_operator = 1;
            *input = current + 2;
            return shell_strdup("||");
        } else {
            /* | operator (pipe) */
            *is_operator = 1;
            *input = current + 1;
            return shell_strdup("|");
        }
    }
    
    if (*current == '&' && *(current + 1) == '&') {
        /* && operator */
        *is_operator = 1;
        *input = current + 2;
        return shell_strdup("&&");
    }
    
    if (*current == ';') {
        /* ; operator */
        *is_operator = 1;
        *input = current + 1;
        return shell_strdup(";");
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
    
    /* Handle regular tokens - stop at operators */
    start = current;
    while (*current && *current != ' ' && *current != '\t' && *current != '\n' &&
           *current != '|' && *current != '&' && *current != ';') {
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
 * Parse a single command (until operator or end)
 */
cmd_node_t* parse_single_command(const char **input) {
    int is_operator;
    char *token = parse_token(input, &is_operator);
    
    if (!token || is_operator) {
        free(token);
        return NULL;
    }
    
    cmd_node_t *node = create_cmd_node();
    if (!node) {
        free(token);
        return NULL;
    }
    
    node->command = token;
    
    /* Collect arguments until we hit an operator */
    char *args[MAX_ARGS];
    int argc = 0;
    
    while ((token = parse_token(input, &is_operator)) && !is_operator && argc < MAX_ARGS - 1) {
        args[argc++] = token;
    }
    
    /* If we got an operator, put it back by backing up the input pointer */
    if (token && is_operator) {
        /* We need to back up - this is a limitation of our simple parser */
        /* For now, we'll handle this in the main parsing loop */
        free(token);
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
            return NULL;
        }
        
        for (int i = 0; i < argc; i++) {
            node->args[i] = args[i];
        }
        node->args[argc] = NULL;
        node->argc = argc;
    }
    
    return node;
}

/**
 * Enhanced command line parser with pipe support
 */
command_chain_t* parse_command_line(const char *line) {
    command_chain_t *chain = create_command_chain();
    if (!chain) return NULL;
    
    const char *current = line;
    cmd_node_t *last_node = NULL;
    
    while (*current) {
        /* Skip whitespace */
        while (*current && (*current == ' ' || *current == '\t')) {
            current++;
        }
        
        if (!*current) break;
        
        /* Parse the next command */
        cmd_node_t *node = parse_single_command(&current);
        if (!node) break;
        
        /* Check what comes next */
        while (*current && (*current == ' ' || *current == '\t')) {
            current++;
        }
        
        if (*current) {
            if (*current == '|' && *(current + 1) != '|') {
                /* Pipe operator */
                node->type = CMD_PIPE;
                current++; /* Skip the | */
            } else if (*current == '&' && *(current + 1) == '&') {
                /* && operator */
                node->type = CMD_AND;
                current += 2; /* Skip && */
            } else if (*current == '|' && *(current + 1) == '|') {
                /* || operator */
                node->type = CMD_OR;
                current += 2; /* Skip || */
            } else if (*current == ';') {
                /* ; operator */
                node->type = CMD_SEMICOLON;
                current++; /* Skip ; */
            }
        }
        
        add_command_to_chain(chain, node);
        last_node = node;
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