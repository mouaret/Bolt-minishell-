import { spawn } from 'child_process';
import { promises as fs } from 'fs';
import { createReadStream, createWriteStream } from 'fs';
import path from 'path';

/**
 * Shell command executor
 */
export class ShellExecutor {
  constructor() {
    this.builtinCommands = {
      'cd': this.cd.bind(this),
      'pwd': this.pwd.bind(this),
      'echo': this.echo.bind(this),
      'exit': this.exit.bind(this),
      'export': this.export.bind(this),
      'unset': this.unset.bind(this),
      'env': this.env.bind(this),
    };
    
    this.environment = { ...process.env };
    this.exitCode = 0;
    this.backgroundProcesses = new Set();
  }

  /**
   * Execute a command chain
   */
  async executeChain(commandChain) {
    if (!commandChain.getHead()) {
      return { exitCode: 0, output: '', error: '' };
    }

    const commands = commandChain.toArray();
    let lastExitCode = 0;

    for (const command of commands) {
      try {
        const result = await this.executeCommand(command);
        lastExitCode = result.exitCode;
        
        if (result.output) {
          process.stdout.write(result.output);
        }
        if (result.error) {
          process.stderr.write(result.error);
        }
        
        // Handle command chaining logic (&&, ||, ;)
        if (command.options.operator === '&&' && lastExitCode !== 0) {
          break; // Stop on failure with &&
        }
        if (command.options.operator === '||' && lastExitCode === 0) {
          break; // Stop on success with ||
        }
        
      } catch (error) {
        process.stderr.write(`Error executing command: ${error.message}\n`);
        lastExitCode = 1;
      }
    }

    this.exitCode = lastExitCode;
    return { exitCode: lastExitCode };
  }

  /**
   * Execute a single command
   */
  async executeCommand(commandNode) {
    const { command, args, pipe, background } = commandNode;

    // Handle built-in commands
    if (this.builtinCommands[command]) {
      return await this.executeBuiltin(command, args, commandNode);
    }

    // Handle piped commands
    if (pipe) {
      return await this.executePipe(commandNode, pipe);
    }

    // Handle regular external commands
    return await this.executeExternal(commandNode);
  }

  /**
   * Execute built-in commands
   */
  async executeBuiltin(command, args, commandNode) {
    try {
      const result = await this.builtinCommands[command](args, commandNode);
      return {
        exitCode: result.exitCode || 0,
        output: result.output || '',
        error: result.error || ''
      };
    } catch (error) {
      return {
        exitCode: 1,
        output: '',
        error: `${command}: ${error.message}\n`
      };
    }
  }

  /**
   * Execute external commands
   */
  async executeExternal(commandNode) {
    return new Promise((resolve) => {
      const { command, args, background, inputRedirect, outputRedirect, appendRedirect, errorRedirect } = commandNode;
      
      const spawnOptions = {
        env: this.environment,
        stdio: ['inherit', 'inherit', 'inherit']
      };

      // Handle redirections
      if (inputRedirect) {
        spawnOptions.stdio[0] = createReadStream(inputRedirect);
      }
      if (outputRedirect) {
        spawnOptions.stdio[1] = createWriteStream(outputRedirect);
      }
      if (appendRedirect) {
        spawnOptions.stdio[1] = createWriteStream(appendRedirect, { flags: 'a' });
      }
      if (errorRedirect) {
        spawnOptions.stdio[2] = createWriteStream(errorRedirect);
      }

      const child = spawn(command, args, spawnOptions);

      if (background) {
        this.backgroundProcesses.add(child.pid);
        console.log(`[${child.pid}] ${command} ${args.join(' ')}`);
        resolve({ exitCode: 0, output: '', error: '' });
        return;
      }

      let output = '';
      let error = '';

      if (child.stdout) {
        child.stdout.on('data', (data) => {
          output += data.toString();
        });
      }

      if (child.stderr) {
        child.stderr.on('data', (data) => {
          error += data.toString();
        });
      }

      child.on('close', (code) => {
        resolve({
          exitCode: code || 0,
          output,
          error
        });
      });

      child.on('error', (err) => {
        resolve({
          exitCode: 127,
          output: '',
          error: `${command}: command not found\n`
        });
      });
    });
  }

  /**
   * Execute piped commands
   */
  async executePipe(leftCommand, rightCommand) {
    return new Promise((resolve) => {
      const leftProcess = spawn(leftCommand.command, leftCommand.args, {
        env: this.environment,
        stdio: ['inherit', 'pipe', 'inherit']
      });

      const rightProcess = spawn(rightCommand.command, rightCommand.args, {
        env: this.environment,
        stdio: ['pipe', 'inherit', 'inherit']
      });

      leftProcess.stdout.pipe(rightProcess.stdin);

      let output = '';
      let error = '';

      if (rightProcess.stdout) {
        rightProcess.stdout.on('data', (data) => {
          output += data.toString();
        });
      }

      if (rightProcess.stderr) {
        rightProcess.stderr.on('data', (data) => {
          error += data.toString();
        });
      }

      rightProcess.on('close', (code) => {
        resolve({
          exitCode: code || 0,
          output,
          error
        });
      });

      leftProcess.on('error', (err) => {
        resolve({
          exitCode: 127,
          output: '',
          error: `${leftCommand.command}: command not found\n`
        });
      });
    });
  }

  // Built-in command implementations
  async cd(args) {
    const dir = args[0] || this.environment.HOME || process.cwd();
    try {
      process.chdir(dir);
      return { exitCode: 0 };
    } catch (error) {
      return { exitCode: 1, error: `cd: ${error.message}\n` };
    }
  }

  async pwd() {
    return { exitCode: 0, output: `${process.cwd()}\n` };
  }

  async echo(args) {
    const output = args.join(' ') + '\n';
    return { exitCode: 0, output };
  }

  async exit(args) {
    const code = parseInt(args[0]) || 0;
    process.exit(code);
  }

  async export(args) {
    if (args.length === 0) {
      // Show all environment variables
      const output = Object.entries(this.environment)
        .map(([key, value]) => `${key}=${value}`)
        .join('\n') + '\n';
      return { exitCode: 0, output };
    }

    args.forEach(arg => {
      const [key, value] = arg.split('=');
      if (value !== undefined) {
        this.environment[key] = value;
      }
    });

    return { exitCode: 0 };
  }

  async unset(args) {
    args.forEach(key => {
      delete this.environment[key];
    });
    return { exitCode: 0 };
  }

  async env() {
    const output = Object.entries(this.environment)
      .map(([key, value]) => `${key}=${value}`)
      .join('\n') + '\n';
    return { exitCode: 0, output };
  }

  /**
   * Get the last exit code
   */
  getExitCode() {
    return this.exitCode;
  }

  /**
   * Clean up background processes
   */
  cleanup() {
    this.backgroundProcesses.forEach(pid => {
      try {
        process.kill(pid);
      } catch (error) {
        // Process might already be dead
      }
    });
  }
}