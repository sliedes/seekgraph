#include <sched.h>

#include <iostream>
#include <cinttypes>
#include <random>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <cstring>
#include <cassert>
#include <fstream>
#include <cmath>

constexpr size_t SIZE = 20*1024*1024*1024ULL;
constexpr size_t RES = 512;
constexpr size_t BLOCK = 32786;

using std::atomic;
using std::cerr;
using std::cout;
using std::endl;
using std::numeric_limits;
using std::ofstream;

typedef uint8_t element_type;

element_type *data = nullptr;

// var = variance
std::vector<unsigned long long> counts, min_cycles, total_cycles;
std::vector<double> sumsq_diff_mean_cycles;

static inline uint64_t rdtsc()
{
  unsigned int hi, lo;
  __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
  return ((uint64_t)hi << 32) | lo;
}

static size_t quant(size_t idx) {
    assert(idx >= 0);
    assert(idx < SIZE);
    return double(idx)/SIZE*RES;
}

static void run_some(size_t seed) {
    std::uniform_int_distribution<size_t> dist(0, SIZE-1);
    std::mt19937_64 rnd(seed);

    uint32_t times[BLOCK];
    constexpr auto maxval = numeric_limits<uint32_t>::max();

    for (size_t i=0; i<BLOCK; i++) {
	size_t p = dist(rnd);
	auto start = rdtsc();
	++data[p];
	auto end = rdtsc();
	auto val = end-start;
	if (val > maxval) {
	    cerr << "Too big value " << val << "." << endl;
	    val = 0;
	}
	times[i] = val;
    }

    rnd.seed(seed);

    size_t prevq = 0;
    for (size_t i=0; i<BLOCK; i++) {
	size_t p = dist(rnd), q = quant(p);
	if (i != 0) {
	    size_t idx = prevq*RES + q;
	    auto newn = ++counts[idx];
	    auto cycles = times[i];
	    if (newn == 1) {
		// first sample here
		total_cycles[idx] = min_cycles[idx] = cycles;
		sumsq_diff_mean_cycles[idx] = 0;
	    } else {
		if (cycles < min_cycles[idx])
		    min_cycles[idx] = cycles;
		auto oldmean = double(total_cycles[idx])/(newn-1);
		total_cycles[idx] += cycles;
		auto newmean = double(total_cycles[idx])/newn;
		double newss = sumsq_diff_mean_cycles[idx] + (cycles - oldmean)*(cycles - newmean);
		sumsq_diff_mean_cycles[idx] = newss;
	    }
	}
	prevq = q;
    }
}

static void set_affinity() {
    cpu_set_t c;
    CPU_ZERO(&c);
    CPU_SET(1, &c);
    if (sched_setaffinity(0, sizeof(c), &c) != 0) {
	cerr << "Failed to set affinity." << endl;
	exit(1);
    }
}

static void write_result() {
    {
	ofstream f("size.txt");
	f << SIZE << endl;
    }

    ofstream meanf("mean.txt"), minf("min.txt"), varf("var.txt"), nsf("nsamples.txt");
    for (size_t y=0; y<RES; y++) {
	for (size_t x=0; x<RES; x++) {
	    auto pos = y*RES + x;
	    constexpr double nanv = nan("");
	    double meanv = nanv, varv = nanv;
	    size_t minv = -1;
	    if (counts[pos] > 0) {
		meanv = double(total_cycles[pos])/counts[pos];
		minv = min_cycles[pos];
		varv = sumsq_diff_mean_cycles[pos] / counts[pos];
	    }
	    meanf << meanv << " ";
	    minf << minv << " ";
	    varf << varv << " ";
	    nsf << counts[pos] << " ";
	}
	meanf << "\n";
	minf << "\n";
	varf << "\n";
	nsf << "\n";
    }
}

int main() {
    set_affinity();

    data = new element_type[SIZE];

    counts.resize(RES*RES);
    total_cycles.resize(RES*RES);
    min_cycles.resize(RES*RES);
    sumsq_diff_mean_cycles.resize(RES*RES);

    memset(data, 0, SIZE*sizeof(*data));

    cerr << "Init done." << endl;

    size_t i=0;
    while (true) {
	run_some(i++);
	if (i%10000 == 0) {
	    cerr << "Writing results after " << i << " iterations." << endl;
	    write_result();
	}
    }
}
