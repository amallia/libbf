#include <cuckoo.hpp>
#include <iostream>
#include <cassert>


template<typename T>
inline void do_not_optimize_away(T&& datum) {
    asm volatile("" : "+r" (datum));
}


int main(int argc, char const* argv[]) {

  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " [n] [runs]" << std::endl;
    return 1;
  }

  uint64_t n = std::stoull(argv[1]);
  uint64_t runs = std::stoull(argv[2]);

  bf::cuckoo_filter filter(n, 12);

  for (uint64_t i = 0; i < n; ++i) {
    filter.add(i);
  }
int c =0;
  double avg = 0.0;
  for (uint64_t run = 0; run < runs; ++run) {
    for (uint64_t i = 0; i < n; ++i) {
      auto start = std::chrono::high_resolution_clock::now();
      auto b = filter.lookup(i);
      auto end = std::chrono::high_resolution_clock::now();
      //assert(b);
      if(!b) c++;
      std::chrono::duration<double> elapsed = end - start;
      avg += elapsed.count();
      do_not_optimize_away(b);
    }
  }
  std::cout << "avg. latency time x lookup: " << avg * 1000000 / runs / n
            << " [musec]\n";
std::cout <<"error:"<< c << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  for (uint64_t run = 0; run < runs; ++run) {
    for (uint64_t i = 0; i < n; ++i) {
      auto b = filter.lookup(i);
      //assert(b);
      do_not_optimize_away(b);
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "total time: " << elapsed.count() << " [sec]\n";
  std::cout << "avg. time x run: " << elapsed.count() / runs << " [sec]\n";
  std::cout << "avg. throughput time x lookup: "
            << elapsed.count() / runs / n * 1000000 << " [musec]\n";

  double p = 0.0;
  for (uint64_t run = 0; run < runs; ++run) {
    uint64_t false_positives = 0;
    for (uint64_t i = n; i < 2 * n; ++i) {
      // rand() % n + n
      if (filter.lookup(i)) {
        ++false_positives;
      }
    }
    p += false_positives;
  }
  std::cout << std::fixed << "false positive rate: " << p / runs / n
            << std::endl;

  return 0;
}