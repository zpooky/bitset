# Makefile
#http://www.puxan.com/web/howto-write-generic-makefiles/
# Declaration of variables
CC = g++
CC_FLAGS = -enable-frame-pointers -std=c++17 $(pkg-config --cflags gtest)
LIBS = -lpthread $(pkg-config --lib gtest)

# File names
EXEC = main
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) $(LIBS)

# To obtain object files
%.o: %.cpp
	$(cc) -c $(cc_flags) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)

print:
	echo "$(CC) $(OBJECTS) -o $(EXEC) $(LIBS)"
