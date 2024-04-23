NAME = xsolid

CFLAGS = -Wall -Wextra -Wpedantic -std=c99  -fmax-errors=5 -g -O0 \
	-I/usr/include/vulkan \
	-I/usr/include/cglm
LDFLAGS = -lglfw -lvulkan -lrt -lm -ldl -lpthread -lcglm
CC = gcc
SOURCES = $(wildcard src/*.c)
ifeq ($(OPTION), clang)
	CC := clang
endif

$(NAME): src/main.c
	$(CC) $(CFLAGS) $(SOURCES) -o $(NAME) $(LDFLAGS)

.PHONY: test clean

test: $(NAME)
	./$(NAME)

clean:
	rm -f $(NAME)
