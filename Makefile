NAME = xsolid

CFLAGS = -Wall -Wextra -Wpedantic -std=c99  -fmax-errors=5 -g -O0 \
	-I/usr/include/vulkan \
	-I/usr/include/cglm
LDFLAGS = -lglfw -lvulkan -lrt -lm -ldl -lpthread -lcglm -lstdc++
CC = gcc
SOURCES = $(wildcard src/*.c)
OBJECTS = extern/vma.o

ifeq ($(OPTION), clang)
	CC := clang
endif

$(NAME): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(OBJECTS) -o $(NAME) $(LDFLAGS)

.PHONY: test clean

test: $(NAME)
	./$(NAME)

clean:
	rm -f $(NAME)
