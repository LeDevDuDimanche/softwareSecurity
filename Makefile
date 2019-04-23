CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -fno-stack-protector -z execstack -pthread

include_flag = -I include/

names = client server 
binaries := $(names:%=bin/%)

shared_src := $(wildcard src/*.cpp)
client_only_src := $(wildcard src/client/*.cpp)
server_only_src := $(wildcard src/server/*.cpp)
client_src := $(shared_src) $(client_only_src)
server_src := $(shared_src) $(server_only_src)
all_src := $(shared_src) $(client_only_src) $(server_only_src)

client_obj := $(client_src:src/%.cpp=obj/%.o)
server_obj := $(server_src:src/%.cpp=obj/%.o)
all_dep := $(all_src:src/%.cpp=obj/%.d)

remove := $(binaries) obj/*.[do] $(names:%=obj/%/*.[do])


.PHONY: all clean

all: $(binaries)

bin/client: $(client_obj)
bin/server: $(server_obj)
bin/%:
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(include_flag) -o $@ $^
	@echo "Built $@ successfully!"

obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(include_flag) -MMD -MP -c -o $@ $<

clean:
	rm -f $(remove)
	rm -f bin/test src/*.o src/*/*.o    # TODO: We can remove this line later


-include $(all_dep)
