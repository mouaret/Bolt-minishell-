# Mini Shell Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -g
TARGET = minishell
SRCDIR = .
OBJDIR = obj

# Source files
SOURCES = shell.c command.c executor.c builtins.c
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

# Default target
all: $(TARGET)

# Create object directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

# Clean build files
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Force rebuild
rebuild: clean all

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall (optional)
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: $(TARGET)

# Run tests (placeholder for future test implementation)
test: $(TARGET)
	@echo "Running basic tests..."
	@echo "echo 'Hello World'" | ./$(TARGET)
	@echo "pwd" | ./$(TARGET)

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the mini shell (default)"
	@echo "  clean    - Remove build files"
	@echo "  rebuild  - Clean and build"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized version"
	@echo "  test     - Run basic tests"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  help     - Show this help"

.PHONY: all clean rebuild install uninstall debug release test help