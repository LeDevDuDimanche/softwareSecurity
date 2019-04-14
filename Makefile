CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -I include/

all: bin/client bin/server

bin/client: src/client.cpp include/grass.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

bin/server: src/server.cpp include/grass.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f bin/client bin/server

.PHONY: all clean
