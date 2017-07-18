#include <bf/bloom_filter/cuckoo.hpp>

#include <cassert>
#include <cmath>

namespace bf {

inline uint64_t upperpower2(uint64_t x) {
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  x++;
  return x;
}

cuckoo_filter::cuckoo_filter(size_t capacity, size_t bits_per_item = 12)
  : hasher_(std::minstd_rand0()())
  , bits_per_item_(bits_per_item)
  , num_buckets_(upperpower2(capacity / tagsPerBucket))
  , tagMask((1ULL << bits_per_item) - 1)
{
  double frac = (double)capacity / num_buckets_ / tagsPerBucket;
  if (frac > 0.96) {
    num_buckets_ <<= 1;
  }
  buckets_.resize(num_buckets_, std::move(Bucket(tagsPerBucket*bits_per_item_)));
}

void cuckoo_filter::add(object const& o) {
  size_t i;
  uint32_t tag;

  GenerateIndexTagHash(o, i, tag);

  size_t curindex = i;
  uint32_t curtag = tag;
  uint32_t oldtag;

  for (uint32_t count = 0; count < maxCuckooCount; count++) {
    bool kickout = count > 0;
    oldtag = 0;
    if (InsertTagToBucket(curindex, curtag, kickout, oldtag)) {
      ++num_items_;
      return;
    }
    if (kickout) {
      curtag = oldtag;
    }
    curindex = AltIndex(curindex, curtag);
  }

  throw std::runtime_error("No space :)");
}

size_t cuckoo_filter::lookup(object const& o) const {

  size_t i1, i2;
  uint32_t tag;

  GenerateIndexTagHash(o, i1, tag);

  i2 = AltIndex(i1, tag);
  assert(i1 == AltIndex(i2, tag));

  if (FindTagInBuckets(i1, i2, tag)) {
    return 1;
  } else {
    return 0;
  }


}

void cuckoo_filter::clear() {
  buckets_.clear();
}

//void cuckoo_filter::remove(object const& o) {

//}

} // namespace bf
