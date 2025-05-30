# Target programs
programs := simple_writer.x simple_reader.x test_fs.x tester.x

# File-system library
FSLIB := libfs
FSPATH := ../$(FSLIB)
libfs := $(FSPATH)/$(FSLIB).a

# Directories
EXEC_DIR := exec
OBJ_DIR := obj

# Create directories if they don't exist
$(shell mkdir -p $(EXEC_DIR) $(OBJ_DIR))

# Default rule
all: $(addprefix $(EXEC_DIR)/,$(programs))

# Avoid builtin rules and variables
MAKEFLAGS += -rR

# Don't print the commands unless explicitly requested with `make V=1`
ifneq ($(V),1)
Q = @
V = 0
endif

# Current directory
CUR_PWD := $(shell pwd)

# Define compilation toolchain
CC = gcc

# General gcc options
CFLAGS := -Wall -Werror -pipe

## Debug flag
ifneq ($(D),1)
CFLAGS += -O2
else
CFLAGS += -g
endif

## Include path
CFLAGS += -I$(FSPATH)

## Dependency generation
CFLAGS += -MMD

# Linker options
LDFLAGS := -L$(FSPATH) -lfs

# Application objects to compile
objs := $(addprefix $(OBJ_DIR)/,$(patsubst %.x,%.o,$(programs)))

# Include dependencies
deps := $(objs:.o=.d)
-include $(deps)

# Rule for libfs.a
$(libfs): FORCE
	@echo "MAKE	$@"
	$(Q)$(MAKE) V=$(V) D=$(D) -C $(FSPATH)

# Generic rule for linking final applications
$(EXEC_DIR)/%.x: $(OBJ_DIR)/%.o $(libfs)
	@echo "LD	$@"
	$(Q)$(CC) -o $@ $< $(LDFLAGS)

# Generic rule for compiling objects
$(OBJ_DIR)/%.o: %.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

# Cleaning rule
clean: FORCE
	@echo "CLEAN	$(CUR_PWD)"
	$(Q)$(MAKE) V=$(V) D=$(D) -C $(FSPATH) clean
	$(Q)rm -rf $(OBJ_DIR) $(EXEC_DIR)

# Keep object files around
.PRECIOUS: $(OBJ_DIR)/%.o
.PHONY: FORCE
FORCE:

#usage: make test args="<command> <diskname>"
test:
	./$(EXEC_DIR)/tester.x $(args)
	
