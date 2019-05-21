CXX		:= g++
C_FLAGS := -Wall -Wextra -std=gnu++17

BINDIR	:= bin
OBJDIR	:= obj

LIBRARIES	:= -lSDL

ifeq ($(OS),Windows_NT)
EXECUTABLE	:= main.exe
else
EXECUTABLE	:= main
endif

INCLUDE	:= -Iinclude -Iinclude/gpu

SRC := \
	src/main.cpp \
	src/frontend.cpp \
	src/gpu/rasterizer.cpp

OBJ := $(addprefix $(OBJDIR)/, $(SRC:.cpp=.o))

all: $(BINDIR)/$(EXECUTABLE)

clean:
	rm $(BINDIR)/$(EXECUTABLE)
	rm -rf $(OBJDIR)/

run: all
	./$(BINDIR)/$(EXECUTABLE)

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(C_FLAGS) $(INCLUDE) -c $< -o $@

$(BINDIR)/$(EXECUTABLE): $(OBJ)
	$(CXX) $(C_FLAGS) $(INCLUDE) $^ -o $@ $(LIBRARIES)