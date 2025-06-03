# Target programs
programs := simple_writer.x simple_reader.x test_fs.x tester.x disk_creator.x read_write.x file_allocation_test.x

# File-system library
FSLIB := libfs
FSPATH := ../$(FSLIB)
libfs := $(FSPATH)/$(FSLIB).a

# Default rule
all: $(programs)

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
CC	= gcc

# General gcc options
CFLAGS	:= -Wall -Werror
CFLAGS	+= -pipe
## Debug flag
ifneq ($(D),1)
CFLAGS	+= -O2
else
CFLAGS	+= -g
endif
## Include path
CFLAGS 	+= -I$(FSPATH)
## Dependency generation
CFLAGS	+= -MMD -MP

# Linker options
LDFLAGS := -L$(FSPATH) -lfs

# Application objects to compile
objs := $(addprefix obj/, $(patsubst %.x,%.o,$(programs)))

# Include dependencies
deps := $(objs:.o=.d)
-include $(deps)

# Ensure obj directory exists
obj:
	mkdir -p obj

# Rule for libfs.a
$(libfs): FORCE
	@echo "MAKE	$@"
	$(Q)$(MAKE) V=$(V) D=$(D) -C $(FSPATH)

# Generic rule for linking final applications
%.x: obj/%.o $(libfs)
	@echo "LD	$@"
	$(Q)$(CC) -o $@ $< $(LDFLAGS)

# Generic rule for compiling objects
obj/%.o: %.c | obj
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

# Cleaning rule
clean: FORCE
	@echo "CLEAN	$(CUR_PWD)"
	$(Q)$(MAKE) V=$(V) D=$(D) -C $(FSPATH) clean
	$(Q)rm -rf obj $(programs)

# Keep object files around
.PRECIOUS: obj/%.o
.PHONY: FORCE
FORCE:

#usage: make test args="<command> <diskname>"
test:
	./tester.x $(args)
	
