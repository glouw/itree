BIN      := itree
CXX      := clang++
CXXFLAGS := $(shell llvm-config --cxxflags) -Wshadow -Wall -pedantic -Wextra -g -O2 -MMD -MP
LDFLAGS  := $(shell llvm-config --ldflags)
LDLIBS   := $(shell llvm-config --system-libs --libs all) -lclang-cpp -lreadline -lcurl
SRC      := $(wildcard *.cc)
OBJ      := $(SRC:.cc=.o)
DEP      := $(OBJ:.o=.d)

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEP)

.PHONY: clean
clean:
	rm -f $(OBJ) $(DEP) $(BIN)
	make clean -C example
