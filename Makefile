CC = g++

CC_INCLUDE = -I/clinic/2014/sandia14/tbb-4.3/include
LD_FLAGS = -L/clinic/2014/sandia14/tbb-4.3/lib64 -ltbb

TARGETS = Main0 Main1

all: $(TARGETS)

Main0: Main0.cc
	$(CC) $< -o $@ -O3 $(CC_INCLUDE) $(LD_FLAGS) -Wall -Wextra -std=c++11

Main1: Main1.cc
	$(CC) $< -o $@ -O3 $(CC_INCLUDE) $(LD_FLAGS) -Wall -Wextra -std=c++11
