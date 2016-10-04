CFLAGS=-g -O3 -Wall -std=c++14
LDFLAGS=

seekgraph: seekgraph.cpp
	g++ $< -o $@ $(CFLAGS) $(LDFLAGS)
