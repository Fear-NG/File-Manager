CC = gcc
CFLAGS = -Wall -Wextra -g -O2
TARGET = file_manager

SRCS = main.c \
       tui_core.c \
       tui_dialogs.c \
       file_crud.c \
       file_preview.c \
       file_search.c \
       file_list.c \
       ui_views.c \
       modals.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
