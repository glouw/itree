BIN      := llmv
CXX      := clang++
CXXFLAGS := -Wshadow -Wall -pedantic -Wextra -g -O0 -MMD -MP $(shell llvm-config --cxxflags)
LDFLAGS  := -lclang-cpp $(shell llvm-config --ldflags)
LDLIBS   := -lclang-cpp $(shell llvm-config --ldflags --system-libs --libs all)
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
