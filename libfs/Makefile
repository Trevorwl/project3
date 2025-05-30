CC = gcc
CFLAGS = -Wall -Werror -MMD -MP

lib := libfs.a

SRCS = disk.c fs.c fdTable.c utilities.c
OBJ_DIR = obj
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))
DEPS = $(OBJS:.o=.d)

all: $(lib)

$(lib): $(OBJS)
	ar rcs $(lib) $(OBJS)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(lib)

-include $(DEPS)