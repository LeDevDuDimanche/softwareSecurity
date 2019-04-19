CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -fno-stack-protector -z execstack -lpthread
INCLUDE = -I include/

SHARED_SRC := $(wildcard src/*.cpp)
CLIENT_SRC := $(SHARED_SRC) $(wildcard src/client/*.cpp)
SERVER_SRC := $(SHARED_SRC) $(wildcard src/server/*.cpp)

CLIENT_OBJ := $(CLIENT_SRC:src/%.cpp=obj/%.o)
SERVER_OBJ := $(SERVER_SRC:src/%.cpp=obj/%.o)


.PHONY: all clean

all: bin/client bin/server

bin/client: $(CLIENT_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $^ -o $@
	@echo "Compiled $@ successfully!"

bin/server: $(SERVER_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $^ -o $@
	@echo "Compiled $@ successfully!"

obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) $< -o $@

clean:
	rm -f bin/client bin/server bin/test obj/*.o obj/*/*.o
	rm -f src/*.o src/*/*.o    # TODO: We can remove this line later
