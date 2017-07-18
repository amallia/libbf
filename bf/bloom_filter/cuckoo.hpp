#ifndef BF_BLOOM_FILTER_CUCKOO_HPP
#define BF_BLOOM_FILTER_CUCKOO_HPP

#include <bf/bitvector.hpp>
#include <bf/bloom_filter.hpp>
#include <bf/hash.hpp>
#include <iostream>
#include <random>

namespace bf {

struct Bucket {
  Bucket(size_t size) : bits_(size) {
  }
  bitvector bits_;
} __attribute__((__packed__));

/// The basic Bloom filter.
///
/// @note This Bloom filter does not use partitioning because it results in
/// slightly worse performance because partitioned Bloom filters tend to have
/// more 1s than non-partitioned filters.
class cuckoo_filter : public bloom_filter {

  inline size_t IndexHash(uint32_t hv) const {
    return hv & (num_buckets_ - 1);
  }

  inline uint32_t TagHash(uint32_t hv) const {
    uint32_t tag;
    tag = hv & ((1ULL << bits_per_item_) - 1);
    tag += (tag == 0);
    return tag;
  }

  inline void GenerateIndexTagHash(const object& item, size_t& index,
                                   uint32_t& tag) const {
    const uint64_t hash = hasher_(item);
    index = IndexHash(hash >> 32);
    tag = TagHash(hash);
  }

  inline size_t AltIndex(const size_t index, const uint32_t tag) const {
    return IndexHash((uint32_t)(index ^ (tag * 0x5bd1e995)));
  }

  inline uint32_t ReadTag(const size_t i, const size_t j) const {
    const bitvector bits = buckets_[i].bits_;
    uint32_t tag = 0;
    for (size_t k = j; k < j + bits_per_item_; ++k) {
      tag = (tag << 1) + bits[k];
    }
    return tag;
  }

  inline void WriteTag(const size_t i, const size_t j, const uint32_t t) {
    bitvector& bits = buckets_[i].bits_;
    uint32_t tag = t & tagMask;
    for (size_t k = j; k < j + bits_per_item_; ++k) {
      auto pos = bits_per_item_ - k - j - 1;
      int b = (tag & (1 << pos)) >> pos;
      bits.set(k, b);
    }
  }

  inline bool InsertTagToBucket(const size_t i, const uint32_t tag,
                                const bool kickout, uint32_t& oldtag) {
    for (size_t j = 0; j < tagsPerBucket; j++) {
      if (ReadTag(i, j) == 0) {
        WriteTag(i, j, tag);
        return true;
      }
    }
    if (kickout) {
      size_t r = rand() % tagsPerBucket;
      oldtag = ReadTag(i, r);
      WriteTag(i, r, tag);
    }
    return false;
  }

  inline bool FindTagInBuckets(const size_t i1, const size_t i2,
                               const uint32_t tag) const {
    for (size_t j = 0; j < tagsPerBucket; j++) {
      if ((ReadTag(i1, j) == tag) || (ReadTag(i2, j) == tag)) {
        return true;
      }
    }
    return false;
  }

  static constexpr size_t tagsPerBucket = 4;
  static constexpr size_t maxCuckooCount = 500 * 1000;

public:
  cuckoo_filter(size_t capacity, size_t bits_per_item);

  using bloom_filter::add;
  using bloom_filter::lookup;

  virtual void add(object const& o) override;
  virtual size_t lookup(object const& o) const override;
  virtual void clear() override;

  // void remove(object const& o);

private:
  default_hash_function hasher_;
  size_t bits_per_item_;
  size_t num_buckets_;
  size_t num_items_;
  uint32_t tagMask;
  std::vector<Bucket> buckets_;
};

} // namespace bf

#endif
