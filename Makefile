# Makefile for the TISA-16 assembler and simulator
#
#   make          -> build the programs "asm" and "sim"
#   make test     -> build and run all tests (data structures + application)
#   make bench    -> compare hash map and linear list lookups (challenge)
#   make clean    -> delete everything that was compiled
#


CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g

# the application code shared by both programs and the tests
APP_SOURCES = app/Format.cpp app/Isa.cpp app/Tokenizer.cpp app/Expression.cpp \
              app/Macro.cpp app/Assembler.cpp app/Simulator.cpp app/Debugger.cpp app/Profiler.cpp app/Pipeline.cpp
DS_SOURCES = ds/HashSet.cpp
HEADERS = ds/*.h app/*.h

all: asm sim

asm: app/asm_main.cpp $(APP_SOURCES) $(DS_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o asm app/asm_main.cpp $(APP_SOURCES) $(DS_SOURCES)

sim: app/sim_main.cpp $(APP_SOURCES) $(DS_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sim app/sim_main.cpp $(APP_SOURCES) $(DS_SOURCES)

test_ds: tests/test_ds.cpp $(DS_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o test_ds tests/test_ds.cpp $(DS_SOURCES)

test_app: tests/test_app.cpp $(APP_SOURCES) $(DS_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o test_app tests/test_app.cpp $(APP_SOURCES) $(DS_SOURCES)

test: test_ds test_app
	./test_ds
	./test_app

bench: bench/bench_symbols.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -O2 -o bench_symbols bench/bench_symbols.cpp
	./bench_symbols

clean:
	rm -f asm sim test_ds test_app bench_symbols

.PHONY: all test bench clean
