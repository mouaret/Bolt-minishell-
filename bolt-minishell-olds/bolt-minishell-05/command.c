#include "shell.h"

/**
 * Portable string duplication function
 */
char* shell_strdup(const char *s) {
    if (!s) return NULL;
    
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (!dup) {
        perror("malloc");
        return NULL;
    }
    
    memcpy(dup, s, len);
    return dup;
}

/**
 * Create a new command chain
 */
command_chain_t* create_command_chain(void) {
    command_chain_t *chain = malloc(sizeof(command_chain_t));
    if (!chain) {
        perror("malloc");
        return NULL;
    }
    
    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
    
    return chain;
}

/**
 * Create a new command node
 */
cmd_node_t* create_cmd_node(void) {
    cmd_node_t *node = malloc(sizeof(cmd_node_t));
    if (!node) {
        perror("malloc");
        return NULL;
    }
    
    node->command = NULL;
    node->args = NULL;
    node->argc = 0;
    node->type = CMD_SIMPLE;
    node->next = NULL;
    node->input_file = NULL;
    node->output_file = NULL;
    node->append_output = 0;
    node->background = 0;
    
    return node;
}

/**
 * Add a command node to the chain
 */
void add_command_to_chain(command_chain_t *chain, cmd_node_t *node) {
    if (!chain || !node) return;
    
    if (chain->head == NULL) {
        chain->head = node;
        chain->tail = node;
    } else {
        chain->tail->next = node;
        chain->tail = node;
    }
    
    chain->count++;
}

/**
 * Free a command node and its resources
 */
void free_cmd_node(cmd_node_t *node) {
    if (!node) return;
    
    free(node->command);
    
    if (node->args) {
        for (int i = 0; i < node->argc; i++) {
            free(node->args[i]);
        }
        free(node->args);
    }
    
    free(node->input_file);
    free(node->output_file);
    free(node);
}

/**
 * Free entire command chain
 */
void free_command_chain(command_chain_t *chain) {
    if (!chain) return;
    
    cmd_node_t *current = chain->head;
    while (current) {
        cmd_node_t *next = current->next;
        free_cmd_node(current);
        current = next;
    }
    
    free(chain);
}

/**
 * Copy arguments array
 */
char** copy_args(char **args, int argc) {
    if (!args || argc == 0) return NULL;
    
    char **new_args = malloc((argc + 1) * sizeof(char*));
    if (!new_args) {
        perror("malloc");
        return NULL;
    }
    
    for (int i = 0; i < argc; i++) {
        new_args[i] = shell_strdup(args[i]);
        if (!new_args[i]) {
            perror("shell_strdup");
            /* Clean up already allocated strings */
            for (int j = 0; j < i; j++) {
                free(new_args[j]);
            }
            free(new_args);
            return NULL;
        }
    }
    new_args[argc] = NULL;
    
    return new_args;
}