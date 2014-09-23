// -*- C++ -*-
// Main0.cc
// some beginning tbb syntax

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <array>
#include <chrono>

// header files for tbb
#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_for.h>
#include <tbb/task_scheduler_init.h>
#include "tbb/atomic.h"

// reduce the amount of typing we have to do for timing things
using std::chrono::high_resolution_clock;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::size_t;
using std::vector;
using std::array;
using tbb::atomic;

class TbbOutputter {
public:

  unsigned long * input_;

  atomic<unsigned long> * result_;

  unsigned long numBuckets_;
  unsigned long size_;

  TbbOutputter(unsigned long * input, atomic<unsigned long> * results,
    unsigned long numBuckets, unsigned long size)
    : input_(input), result_(results), numBuckets_(numBuckets), size_(size) {
  }

  TbbOutputter(const TbbOutputter & other,
               tbb::split)
               : input_(other.input_), result_(other.result_),
                  numBuckets_(other.numBuckets_), size_(other.size_){
    //printf("split copy constructor called\n");
  }

  void operator()(const tbb::blocked_range<size_t> & range) {
    //printf("TbbOutputter asked to process range from %7zu to %7zu\n",
           //range.begin(), range.end());

    unsigned long bucketSize = size_/numBuckets_;
    for(unsigned long i=range.begin(); i!= range.end(); ++i ) {
      (result_ + (i/bucketSize))->fetch_and_increment();
    }
  }

  void join(const TbbOutputter & other) {

  }

private:
  TbbOutputter();

};

int main() {


  // a couple of inputs.  change the numberOfIntervals to control the amount
  //  of work done
  const unsigned long numberOfElements = 1e6;

  // The number of buckets in our histogram
  const unsigned long numberOfBuckets = 1e3;

  const unsigned long bucketSize = numberOfElements / numberOfBuckets;
  // these are c++ timers...for timing
  high_resolution_clock::time_point tic;
  high_resolution_clock::time_point toc;

  fprintf(stderr,"Creating the input vector \n");
  unsigned long * inputs = new unsigned long[numberOfElements];

  for(unsigned long i = 0; i < numberOfElements; ++i) {
    inputs[i] = i;
  }


  // first, do the serial calculation
  vector<unsigned long> serialResults (numberOfBuckets);

  tic = high_resolution_clock::now();

  for(unsigned long i = 0; i < numberOfElements; ++i) {
    unsigned long in = inputs[i];
    serialResults[in/bucketSize] += 1;
  }


  toc = high_resolution_clock::now();
  const double serialElapsedTime =
    duration_cast<duration<double> >(toc - tic).count();


  for(unsigned long i = 0; i < numberOfBuckets; ++i) {
    if(serialResults[i] != bucketSize) {
      fprintf(stderr, "one of the buckets, %lu, has the wrong value, %lu instead of %lu \n", i, serialResults[i], bucketSize);
      exit(1);
    }
  }

  // we will repeat the computation for each of the numbers of threads
  vector<unsigned int> numberOfThreadsArray;
  numberOfThreadsArray.push_back(1);
  numberOfThreadsArray.push_back(2);
  numberOfThreadsArray.push_back(4);

  // for each number of threads
  for (unsigned int numberOfThreadsIndex = 0;
       numberOfThreadsIndex < numberOfThreadsArray.size();
       ++numberOfThreadsIndex) {
    const unsigned int numberOfThreads =
      numberOfThreadsArray[numberOfThreadsIndex];

    // initialize tbb's threading system for this number of threads
    tbb::task_scheduler_init init(numberOfThreads);

    printf("processing with %2u threads:\n", numberOfThreads);

    atomic<unsigned long> * results = new atomic<unsigned long>[numberOfBuckets];
    bzero(results, sizeof(atomic<unsigned long>) * numberOfElements);
    TbbOutputter tbbOutputter(inputs, results, numberOfBuckets, numberOfElements);

    // start timing
    tic = high_resolution_clock::now();
    // dispatch threads
    parallel_reduce(tbb::blocked_range<size_t>(0, numberOfElements),
                    tbbOutputter);
    // stop timing
    toc = high_resolution_clock::now();
    const double threadedElapsedTime =
      duration_cast<duration<double> >(toc - tic).count();


    atomic<unsigned long> * vectorResults = tbbOutputter.result_;


    for(unsigned long i = 0; i < numberOfBuckets; ++i) {
      if(vectorResults[i] != bucketSize) {
        fprintf(stderr, "one of the buckets, %lu, has the wrong value, %lu instead of %lu \n", i, vectorResults[i], bucketSize);
        exit(1);
      }
    }



    delete results;

    // output speedup
    printf("%3u : time %8.2e speedup %8.2e (%%%5.1f of ideal)\n",
           numberOfThreads,
           threadedElapsedTime,
           serialElapsedTime / threadedElapsedTime,
           100. * serialElapsedTime / threadedElapsedTime / numberOfThreads);


  }

  delete inputs;
  return 0;

}
