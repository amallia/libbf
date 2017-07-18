#include <cuckoo.hpp>

int main(int argc, char const *argv[])
{
  bf::cuckoo_filter bf(10, 8);
  bf.add(12);

    assert(bf.lookup(12) == 1);
  std::cout <<  bf.lookup(12) << std::endl;
  return 0;
}