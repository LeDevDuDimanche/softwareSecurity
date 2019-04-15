CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -I include/

OBJDIR = objs

all: bin/client bin/server

bin/client: src/client.o src/grass.o
	$(CXX) $(CXXFLAGS) $< -o $@

bin/server: src/server.o src/grass.o src/conn.o src/commands/pathvalidate.o src/commands/parsing.o src/commands/systemcmd.o src/commands/commands.o
	$(CXX) $(CXXFLAGS) $< -o $@


%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f src/*.o bin/client bin/server bin/test src/commands/*.o

.PHONY: all clean
