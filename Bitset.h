#ifndef SP_CONCURRENT_BITSET_H
#define SP_CONCURRENT_BITSET_H

#include <array>
#include <atomic>
#include <bitset>
#include <cmath>
#include <iostream>
#include <type_traits>

namespace sp {

template <size_t T_Size, typename Byte_t = uint8_t>
class Bitset {
public:
  static constexpr size_t npos = T_Size;

private:
  using Entry_t = std::atomic<Byte_t>;
  static constexpr size_t bits = sizeof(Byte_t) * 8;
  static constexpr size_t T_Words = size_t(std::ceil(double(T_Size) / bits));
  static constexpr Byte_t one_ = Byte_t(1) << size_t(bits - 1); // 10000...
  //
  static_assert(std::is_scalar<Byte_t>::value,
                "Backing structure is required to be a scalar");
  static_assert(std::is_integral<Byte_t>::value,
                "Backing structure is required to be a integral");
  static_assert(T_Size % 8 == 0, "Size should be evenly divisable with 8");

  /**
   * |word|word|...|
   * ^         ^
   * |low bit  |high bit
   */
  struct Entry {
  private:
  public:
    std::array<Entry_t, T_Words> m_data;

    Entry() noexcept //
        : m_data() {
    }

    explicit Entry(const std::bitset<T_Size> &init) noexcept //
        : Entry() {
      transfer(init);
    }

    explicit Entry(bool v) noexcept //
        : Entry() {
      init_with(v ? ~Byte_t(0) : Byte_t(0));
    }

  private:
    constexpr size_t
    byte_index(size_t idx) const noexcept {
      return size_t(idx / bits);
    }

    Entry_t &
    word_for(size_t byteIdx) noexcept {
      return m_data[byteIdx];
    }

    const Entry_t &
    word_for(size_t byteIdx) const noexcept {
      return m_data[byteIdx];
    }

    void
    store(size_t wordIdx, Byte_t value) noexcept {
      auto &entry = word_for(wordIdx);
      entry.store(value);
    }

    using ttttt = unsigned long long;

    void
    init_with(Byte_t def) noexcept {
      for (size_t idx(0); idx < T_Words; ++idx) {
        auto &word = word_for(idx);
        word.store(def);
      }
    }

    void
    transfer(const std::bitset<T_Size> &init) noexcept {
      Byte_t word(0);
      size_t entryIdx(0);
      size_t i(0);
      for (; i < init.size(); ++i) {
        if (init[i]) {
          size_t bitoffset = (bits - 1) - (i % bits);
          Byte_t cbits = Byte_t(1) << bitoffset;
          word = word | cbits;
        }

        if (size_t(i + 1) % bits == size_t(0)) {
          store(entryIdx++, word);
          word = Byte_t(0);
        }
      }
      if (i % bits != size_t(0)) {
        store(entryIdx, word);
      }
    }

  public:
    /**
     * sets or unsets a bit at specified index
     * returns true if the specified bit has to be altered
     * returns false if no change was required
     */
    bool
    set(size_t bitIdx, bool b) noexcept {
      Entry_t &e = word_for(byte_index(bitIdx));

      const Byte_t offset = word_index(bitIdx);
      const Byte_t mask = one_ >> offset;

      auto word_before = e.load();
      Byte_t word;
      do {
        if (b) {
          // printf("word(%d) = word_before(%d) ^ mask(%d)\n",
          //        (word_before | mask), word_before, mask);
          word = word_before | mask;
        } else {
          // printf("word(%d) = word_before(%d) & ~mask(%d)\n",
          //        (word_before & (~mask)), word_before, mask);
          word = word_before & (~mask);
        }
        // since we don't need to update if they are the same
        if (word == word_before) {
          // check that *word_before* has not allready been set
          // if we did not change anything
          return false;
        }
        /**
         * if the compare exchange fails the word_before will be updated with
         * the current value
         */
      } while (!e.compare_exchange_strong(word_before, word));
      return true;
    }

    constexpr Byte_t
    word_index(size_t bitIdx) const noexcept {
      return bitIdx - ttttt(ttttt(bitIdx / bits) * bits);
    }

    bool
    test(size_t bitIdx) const noexcept {
      size_t byteIdx = byte_index(bitIdx);
      const Entry_t &e = word_for(byteIdx);

      auto wordIdx = word_index(bitIdx);
      const Byte_t mask = one_ >> wordIdx;

      auto word = e.load();
      return Byte_t(word & mask) != Byte_t(0);
    }

    Byte_t
    mask_right(Byte_t idx) const noexcept {
      const Byte_t b = ~Byte_t(0);
      const Byte_t res = b >> idx;
      return res;
    }

    /* 11111111_11111111|65535
     * 11111111_11111110|65534
     * 01111111_11111111|32767
     * 10000000_00000000|32768
     * 00000000_00000001|1
     */
    bool
    all(size_t bitIdx, Byte_t test) const noexcept {
      size_t idx = byte_index(bitIdx);
      Byte_t wordIdx = word_index(bitIdx);

      Byte_t mask = test & mask_right(wordIdx);

      for (; idx < T_Words; ++idx) {
        auto &word = word_for(idx);
        Byte_t current = word.load();

        current = current & mask_right(wordIdx);

        if (current != mask) {
          return false;
        }
        mask = test;
        wordIdx = Byte_t(0);
      }
      return true;
    }

    size_t
    bit_index(size_t byteIdx, Byte_t wordIdx) const noexcept {
      return size_t(byteIdx * bits) + wordIdx;
    }

    size_t
    find_first(size_t bitIdx, bool find) const noexcept {
      // TODO
      size_t byteIdx = byte_index(bitIdx);
      auto wordIdx = word_index(bitIdx);

      const Byte_t mask = find ? Byte_t(0) : ~Byte_t(0);

      for (; byteIdx < T_Words; ++byteIdx) {
        auto &wrd = word_for(byteIdx);
        const Byte_t &word = wrd.load();

        if (mask != word) {
          for (size_t bit = wordIdx; bit < bits; ++bit) {

            Byte_t vmask = one_ >> bit;
            Byte_t cmp = find ? vmask : Byte_t(0); // TODO

            if (Byte_t(vmask & word) == cmp) {
              return bit_index(byteIdx, bit);
            }
          }
        }
        wordIdx = Byte_t(0);
      }
      return npos;
    }

    size_t
    swap_first(size_t bitIdx, bool set, size_t limitIdx) noexcept {
      size_t wordIdx = byte_index(bitIdx);
      Byte_t wordBitStart = word_index(bitIdx);
      /**
       * we only need to look in words which is not:
       * all 1 if 'set' is true
       * all 0 if 'set' is false
       */
      const Byte_t mask = set ? ~Byte_t(0) : Byte_t(0);

      const size_t limitWord = byte_index(limitIdx);
      while (wordIdx <= limitWord) {
        auto &current = word_for(wordIdx);
        Byte_t word = current.load(std::memory_order_acquire);

        if (mask != word) {
          size_t bitMax = wordIdx == limitWord ? word_index(limitIdx) : bits;
          for (size_t bit = wordBitStart; bit < bitMax; ++bit) {
            /**
             * Mask for current bit: 0001000
             */
            const Byte_t vmask = one_ >> bit;
            const Byte_t cmp = set ? Byte_t(0) : vmask;

            if (Byte_t(vmask & word) == cmp) {
              Byte_t value = set ? Byte_t(word | vmask)
                                 : Byte_t(word & Byte_t(vmask ^ ~Byte_t(0)));
              if (current.compare_exchange_strong(word, value)) {
                return bit_index(wordIdx, bit);
              }
            }
          } // for
        }
        wordBitStart = Byte_t(0);
        ++wordIdx;
      } // while
      return T_Size;
    }
  };

private:
  Entry m_entry;

public:
  explicit Bitset(const std::bitset<T_Size> &init) noexcept //
      : m_entry{init} {
  }

  Bitset() noexcept //
      : m_entry{} {
  }

  /**
   *  @brief init the bitset with
   *  @param  b  the value to fill with
   */
  explicit Bitset(bool v) noexcept //
      : m_entry(v) {
  }

  Bitset(const Bitset &) = delete;
  Bitset(Bitset &&) = delete;

  Bitset &
  operator=(const Bitset &) = delete;
  Bitset &
  operator=(Bitset &&) = delete;

  ~Bitset() noexcept {
  }

  constexpr size_t
  size() const noexcept {
    return T_Size;
  }

  /**
   *  @brief sets the specified bit to b
   *  @param  bitIdx  The index of a bit.
   *  @return wether b is set to b allready true, otherwise false
   *  @throw  std::out_of_range  If @a pos is bigger the size of the %set.
   */
  bool
  set(size_t bitIdx, bool b) noexcept {
    if (bitIdx >= T_Size) {
      // TODO
    }
    return m_entry.set(bitIdx, b);
  }

  bool
  test(size_t bitIdx) const noexcept {
    if (bitIdx >= T_Size) {
      return false;
    }
    bool ret = m_entry.test(bitIdx);
    return ret;
  }

  bool operator[](size_t bitIdx) const noexcept {
    return test(bitIdx);
  }

  /**
   * for all starting with $bitIndex will be tested
   * against $test if all satisfies return true or else false
   */
  bool
  all(size_t bitIdx, bool test) const noexcept {
    if (bitIdx >= T_Size) {
      return false;
    }
    return m_entry.all(bitIdx, test ? ~Byte_t(0) : Byte_t(0));
  }

  bool
  all(bool test) const noexcept {
    return all(size_t(0), test);
  }

  size_t
  find_first(size_t bitIdx, bool find) const noexcept {
    if (bitIdx >= T_Size) {
      return false;
    }
    return m_entry.find_first(bitIdx, find);
  }

  size_t
  find_first(bool find) const noexcept {
    return find_first(size_t(0), find);
  }

  size_t
  swap_first(size_t idx, bool set, size_t limit) noexcept {
    if (idx >= T_Size) {
      return npos;
    }
    return m_entry.swap_first(idx, set, limit);
  }

  size_t
  swap_first(size_t idx, bool set) noexcept {
    return swap_first(idx, set, T_Size);
  }

  size_t
  swap_first(bool set) noexcept {
    return swap_first(size_t(0), set);
  }

  size_t
  swap_first(bool set, size_t limit) noexcept {
    return swap_first(size_t(0), set, limit);
  }

  std::string
  to_string() {
    // this print in an reverse order to << operator
    std::string res;
    res.reserve(size());
    for (size_t i = 0; i < size(); ++i) {
      char c;
      if (this->test(i)) {
        c = '1';
      } else {
        c = '0';
      }
      res.push_back(c);
    }
    return res;
  }
};

template <size_t size, typename Type>
std::ostream &
operator<<(std::ostream &os, const Bitset<size, Type> &b) {
  for (size_t i = b.size(); i-- > 0;) {
    if (b[i]) {
      os << '1';
    } else {
      os << '0';
    }
  }
  return os;
}
} // namespace sp

#endif
