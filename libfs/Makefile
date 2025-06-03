CC = gcc
CFLAGS = -Wall -Werror -MMD -MP

lib := libfs.a

SRCS = disk.c fs.c fdTable.c utilities.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

all: $(lib)

$(lib): $(OBJS)
	ar rcs $(lib) $(OBJS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *.d $(lib)

-include $(DEPS)