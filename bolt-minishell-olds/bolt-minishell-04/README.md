# Mini Shell - POSIX Compatible Shell Implementation

A lightweight, POSIX-compatible shell implementation in C featuring command chaining, built-in commands, and process management.

## Features

### Core Functionality
- **Chained List Architecture**: Commands stored in linked list structure for flexible execution
- **Built-in Commands**: cd, pwd, echo, echo -n, env, exit
- **External Command Execution**: Fork/exec pattern for running system programs
- **Command Operators**: Support for &&, ||, | (pipe), ; (semicolon)
- **Background Execution**: Commands can run in background with &
- **I/O Redirection**: Input/output redirection support

### Command Types Supported
- Simple commands: `ls -la`
- Conditional execution: `make && ./program`
- Alternative execution: `command1 || command2`  
- Piped commands: `ls | grep txt`
- Sequential commands: `cd /tmp; ls; pwd`

## Architecture

### Chained List Structure
Commands are stored in a linked list where each node contains:
```c
typedef struct cmd_node {
    char *command;              // Command name
    char **args;               // Arguments array
    int argc;                  // Argument count
    cmd_type_t type;          // Command type (AND, OR, PIPE, etc.)
    struct cmd_node *next; // Next command in chain
    // ... redirection and execution flags
} cmd_node_t;
```

### Execution Flow
1. **Parse**: Command line parsed into chained list (your partner's parser)
2. **Execute**: Commands executed sequentially based on type
3. **Cleanup**: Memory and resources freed after execution

## Building

### Requirements
- GCC compiler
- POSIX-compatible system (Linux, macOS, Unix)
- Make utility

### Compilation
```bash
# Build the shell
make

# Debug build
make debug

# Optimized release build  
make release

# Clean build files
make clean

# Rebuild from scratch
make rebuild
```

### Installation
```bash
# Install to /usr/local/bin
make install

# Uninstall
make uninstall
```

## Usage

### Running the Shell
```bash
./minishell
```

### Built-in Commands
- `cd [directory]` - Change directory
- `pwd` - Print working directory  
- `echo [text]` - Print text
- `echo -n [text]` - Print text without newline
- `env` - Show environment variables
- `exit [code]` - Exit shell with optional code

### Command Examples
```bash
# Simple command
$ ls -la

# Conditional execution
$ make && ./program

# Alternative execution  
$ test -f file.txt || echo "File not found"

# Piped commands
$ ps aux | grep bash | wc -l

# Sequential commands
$ cd /tmp; ls; pwd

# Background execution
$ sleep 10 &
```

## Integration with Parser

This executor is designed to work with your partner's parser. The parser should:

1. **Create command chain**: Use `create_command_chain()`
2. **Add commands**: Create nodes with `create_cmd_node()` and add with `add_command_to_chain()`
3. **Set command types**: Assign appropriate `cmd_type_t` values
4. **Handle redirections**: Set input/output file fields
5. **Execute**: Call `execute_command_chain()`

### Parser Integration Example
```c
// In your partner's parser
command_chain_t *chain = create_command_chain();
cmd_node_t *node = create_cmd_node();
node->command = strdup("ls");
node->args = parse_arguments(); // Your parsing logic
node->type = CMD_SIMPLE;
add_command_to_chain(chain, node);

// Execute the chain
execute_command_chain(chain, shell_context);
free_command_chain(chain);
```

## Memory Management

The implementation includes comprehensive memory management:
- **Automatic cleanup**: Command chains and nodes are properly freed
- **Error handling**: Robust error checking and resource cleanup
- **No memory leaks**: All allocated memory is tracked and freed

## Testing

```bash
# Run basic tests
make test

# Manual testing examples
echo "pwd" | ./minishell
echo "echo Hello World" | ./minishell  
echo "ls | wc -l" | ./minishell
```

## File Structure

- `shell.h` - Header with structures and function prototypes
- `shell.c` - Main shell loop and initialization
- `command.c` - Command chain and node management
- `executor.c` - Command execution logic
- `builtins.c` - Built-in command implementations
- `Makefile` - Build configuration
- `README.md` - This documentation

## Contributing

This shell is designed for educational purposes and can be extended with:
- More built-in commands
- Advanced redirection features
- Job control
- Command history
- Tab completion

## License

Educational/Academic use. Feel free to extend and modify for learning purposes.