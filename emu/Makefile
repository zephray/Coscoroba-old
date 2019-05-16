CC		:= gcc
C_FLAGS := -Wall -Wextra

BINDIR	:= bin
OBJDIR	:= obj

LIBRARIES	:= -lSDL

ifeq ($(OS),Windows_NT)
EXECUTABLE	:= main.exe
else
EXECUTABLE	:= main
endif

INCLUDE	:= -Iinclude

SRC := \
	src/main.c \
	src/frontend.c

OBJ := $(addprefix $(OBJDIR)/, $(SRC:.c=.o))

all: $(BINDIR)/$(EXECUTABLE)

clean:
	rm $(BINDIR)/$(EXECUTABLE)
	rm -rf $(OBJDIR)/

run: all
	./$(BINDIR)/$(EXECUTABLE)

$(OBJDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(INCLUDE) -c $< -o $@

$(BINDIR)/$(EXECUTABLE): $(OBJ)
	$(CC) $(C_FLAGS) $(INCLUDE) $^ -o $@ $(LIBRARIES)