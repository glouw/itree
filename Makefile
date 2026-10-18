all:
	clang++ llmv.cc -o llmv $$(llvm-config --cxxflags --ldflags --system-libs --libs all) -lclang-cpp -std=c++20
