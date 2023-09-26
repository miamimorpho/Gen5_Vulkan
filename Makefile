NAME = xsolid

CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -O0
LDFLAGS = -lSDL2 -lvulkan -lrt -lm -ldl -lpthread -lcglm
CC = gcc
ifeq ($(OPTION), clang)
	CC := clang
endif

$(NAME): main.c
	$(CC) $(CFLAGS) -o $(NAME) main.c \
	 pipeline.c memory.c drawing.c model.c camera.c stage1.c $(LDFLAGS)

.PHONY: test clean

test: $(NAME)
	./$(NAME)

clean:
	rm -f $(NAME)
