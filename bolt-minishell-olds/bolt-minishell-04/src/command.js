/**
 * Command node structure for the chained list
 */
export class CommandNode {
  constructor(command, args = [], options = {}) {
    this.command = command;
    this.args = args;
    this.options = options; // For redirections, pipes, etc.
    this.next = null;
    this.pipe = null; // For piped commands
    this.background = false; // For background execution (&)
    this.inputRedirect = null; // For input redirection (<)
    this.outputRedirect = null; // For output redirection (>)
    this.appendRedirect = null; // For append redirection (>>)
    this.errorRedirect = null; // For error redirection (2>)
  }

  /**
   * Set the next command in the chain
   */
  setNext(commandNode) {
    this.next = commandNode;
    return commandNode;
  }

  /**
   * Set pipe to another command
   */
  setPipe(commandNode) {
    this.pipe = commandNode;
    return commandNode;
  }

  /**
   * Get full command string for debugging
   */
  toString() {
    let cmd = `${this.command} ${this.args.join(' ')}`.trim();
    if (this.inputRedirect) cmd += ` < ${this.inputRedirect}`;
    if (this.outputRedirect) cmd += ` > ${this.outputRedirect}`;
    if (this.appendRedirect) cmd += ` >> ${this.appendRedirect}`;
    if (this.errorRedirect) cmd += ` 2> ${this.errorRedirect}`;
    if (this.background) cmd += ' &';
    return cmd;
  }
}

/**
 * Command chain manager
 */
export class CommandChain {
  constructor() {
    this.head = null;
    this.current = null;
  }

  /**
   * Add a command to the chain
   */
  addCommand(command, args = [], options = {}) {
    const node = new CommandNode(command, args, options);
    
    if (!this.head) {
      this.head = node;
      this.current = node;
    } else {
      this.current.setNext(node);
      this.current = node;
    }
    
    return node;
  }

  /**
   * Get the first command in the chain
   */
  getHead() {
    return this.head;
  }

  /**
   * Clear the command chain
   */
  clear() {
    this.head = null;
    this.current = null;
  }

  /**
   * Convert chain to array for easier processing
   */
  toArray() {
    const commands = [];
    let current = this.head;
    
    while (current) {
      commands.push(current);
      current = current.next;
    }
    
    return commands;
  }
}