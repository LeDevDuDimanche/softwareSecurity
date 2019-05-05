CXX = g++
# Extra flags to prevent segfaulting: https://stackoverflow.com/q/35116327
CXXFLAGS = -std=c++11 -Wall -Wextra -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -g -fno-stack-protector -z execstack -m32

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

latex_extensions = aux log pdf

remove := $(binaries) obj/*.[do] $(names:%=obj/%/*.[do])
remove_exploits := $(latex_extensions:%=exploits/exploits.%)
remove_docs := $(latex_extensions:%=docs/documentation.%)


.PHONY: all exploits clean clean-bin clean-exploits clean-docs

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

exploits: exploits/exploits.pdf

exploits/exploits.pdf: exploits/exploits.tex
	pdflatex -output-directory=exploits exploits/exploits

docs: docs/documentation.pdf

docs/documentation.pdf: docs/documentation.tex
	pdflatex -output-directory=docs docs/documentation

clean: clean-bin clean-exploits clean-docs

clean-bin:
	rm -f $(remove)

clean-exploits:
	rm -f $(remove_exploits)

clean-docs:
	rm -f $(remove_docs)


-include $(all_dep)
