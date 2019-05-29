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
	src/gpu/rasterizer.cpp \
	src/gpu/shader.cpp

OBJ := $(addprefix $(OBJDIR)/, $(SRC:.cpp=.o))

all: $(BINDIR)/$(EXECUTABLE)

clean:
	rm -rf $(OBJDIR)/
	rm $(BINDIR)/$(EXECUTABLE)

run: all
	./$(BINDIR)/$(EXECUTABLE)

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(C_FLAGS) $(INCLUDE) -c $< -o $@

$(BINDIR)/$(EXECUTABLE): $(OBJ)
	mkdir -p $(BINDIR)
	$(CXX) $(C_FLAGS) $(INCLUDE) $^ -o $@ $(LIBRARIES)