CXX= g++
CXXFLAGS= -std=c++11

INCLUDE= -I/usr/include/SDL2 -I./include
LIB= -lSDL2 -lSDL2_ttf -lSDL2_image

SRCDIR= src
OBJDIR= obj
BINDIR= bin

OBJS= $(addprefix $(OBJDIR)/, main.o)
EXEC= $(addprefix $(BINDIR)/, fileexplorer)

# CREATE DIRECTORIES (IF DON'T ALREADY EXIST)
mkdirs:= $(shell mkdir -p $(OBJDIR) $(BINDIR))


# BUILD EVERYTHING
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIB)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INCLUDE)


# REMOVE OLD FILES
clean:
	rm -f $(OBJS) $(EXEC)

