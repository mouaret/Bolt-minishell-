import readline from 'readline';
import { CommandChain } from './command.js';
import { ShellExecutor } from './executor.js';

/**
 * Main shell implementation
 */
class MiniShell {
  constructor() {
    this.executor = new ShellExecutor();
    this.rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout,
      prompt: '$ '
    });
    
    this.setupSignalHandlers();
  }

  /**
   * Start the shell
   */
  start() {
    console.log('Mini Shell v1.0 - POSIX Compatible');
    console.log('Type "exit" to quit\n');
    
    this.rl.prompt();
    
    this.rl.on('line', async (input) => {
      await this.processInput(input.trim());
      this.rl.prompt();
    });

    this.rl.on('close', () => {
      console.log('\nGoodbye!');
      this.executor.cleanup();
      process.exit(0);
    });
  }

  /**
   * Process user input
   */
  async processInput(input) {
    if (!input) return;

    try {
      // Simple command parsing (your parser will replace this)
      const commandChain = this.parseInput(input);
      await this.executor.executeChain(commandChain);
    } catch (error) {
      console.error(`Error: ${error.message}`);
    }
  }

  /**
   * Simple command parser (placeholder for your parser)
   * This is where your partner's parser will integrate
   */
  parseInput(input) {
    const chain = new CommandChain();
    
    // Handle basic command separation (;, &&, ||)
    const commands = this.splitCommands(input);
    
    commands.forEach(({ command, operator }) => {
      const parts = this.parseCommand(command);
      if (parts.command) {
        const node = chain.addCommand(parts.command, parts.args, { operator });
        
        // Handle redirections
        if (parts.inputRedirect) node.inputRedirect = parts.inputRedirect;
        if (parts.outputRedirect) node.outputRedirect = parts.outputRedirect;
        if (parts.appendRedirect) node.appendRedirect = parts.appendRedirect;
        if (parts.errorRedirect) node.errorRedirect = parts.errorRedirect;
        if (parts.background) node.background = parts.background;
        
        // Handle pipes
        if (parts.pipe) {
          const pipeParts = this.parseCommand(parts.pipe);
          if (pipeParts.command) {
            const pipeNode = new CommandNode(pipeParts.command, pipeParts.args);
            node.setPipe(pipeNode);
          }
        }
      }
    });
    
    return chain;
  }

  /**
   * Split commands by operators
   */
  splitCommands(input) {
    const commands = [];
    let current = '';
    let operator = null;
    
    for (let i = 0; i < input.length; i++) {
      const char = input[i];
      const nextChar = input[i + 1];
      
      if (char === ';') {
        if (current.trim()) {
          commands.push({ command: current.trim(), operator });
        }
        current = '';
        operator = ';';
      } else if (char === '&' && nextChar === '&') {
        if (current.trim()) {
          commands.push({ command: current.trim(), operator });
        }
        current = '';
        operator = '&&';
        i++; // Skip next character
      } else if (char === '|' && nextChar === '|') {
        if (current.trim()) {
          commands.push({ command: current.trim(), operator });
        }
        current = '';
        operator = '||';
        i++; // Skip next character
      } else {
        current += char;
      }
    }
    
    if (current.trim()) {
      commands.push({ command: current.trim(), operator });
    }
    
    return commands;
  }

  /**
   * Parse a single command
   */
  parseCommand(command) {
    const parts = {
      command: null,
      args: [],
      inputRedirect: null,
      outputRedirect: null,
      appendRedirect: null,
      errorRedirect: null,
      background: false,
      pipe: null
    };

    // Handle background execution
    if (command.endsWith('&')) {
      parts.background = true;
      command = command.slice(0, -1).trim();
    }

    // Handle pipes
    const pipeIndex = command.indexOf('|');
    if (pipeIndex !== -1 && command[pipeIndex + 1] !== '|') {
      parts.pipe = command.substring(pipeIndex + 1).trim();
      command = command.substring(0, pipeIndex).trim();
    }

    // Simple tokenization (your parser will handle this better)
    const tokens = command.split(/\s+/);
    parts.command = tokens[0];
    
    for (let i = 1; i < tokens.length; i++) {
      const token = tokens[i];
      
      if (token === '<' && tokens[i + 1]) {
        parts.inputRedirect = tokens[++i];
      } else if (token === '>' && tokens[i + 1]) {
        parts.outputRedirect = tokens[++i];
      } else if (token === '>>' && tokens[i + 1]) {
        parts.appendRedirect = tokens[++i];
      } else if (token === '2>' && tokens[i + 1]) {
        parts.errorRedirect = tokens[++i];
      } else {
        parts.args.push(token);
      }
    }

    return parts;
  }

  /**
   * Setup signal handlers
   */
  setupSignalHandlers() {
    process.on('SIGINT', () => {
      console.log('\n');
      this.rl.prompt();
    });

    process.on('SIGTERM', () => {
      this.executor.cleanup();
      process.exit(0);
    });
  }
}

// Start the shell if this file is run directly
if (import.meta.url === `file://${process.argv[1]}`) {
  const shell = new MiniShell();
  shell.start();
}

export default MiniShell;