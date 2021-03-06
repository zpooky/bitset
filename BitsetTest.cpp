#include "Bitset.h"
#include "gtest/gtest.h"
#include <iostream>
#include <random>
#include <thread>
#include <unordered_set>

using sp::Bitset;
using std::cout;
using std::endl;

class BitsetTest : public ::testing::TestWithParam<bool> {};

INSTANTIATE_TEST_CASE_P(PreFill, BitsetTest, ::testing::Values(true, false));

TEST_F(BitsetTest, test_empty) {
  constexpr size_t bits = 1024;
  Bitset<bits> b;
  for (size_t i = 0; i < bits; ++i) {
    ASSERT_FALSE(b.test(i));
  }
}

template <size_t bits, typename T>
void
true_set(Bitset<bits, T> &b) {
  for (size_t i = 0; i < bits; ++i) {
    for (size_t a = 0; a < i; ++a) {
      ASSERT_TRUE(b.test(a));
    }
    ASSERT_TRUE(b.set(i, true));
    ASSERT_FALSE(b.set(i, true));
    for (size_t a = bits - 1; a > i; --a) {
      ASSERT_FALSE(b.test(a));
    }
  }
}

template <size_t bits, typename T>
void
false_set(Bitset<bits, T> &b) {
  for (size_t i = 0; i < bits; ++i) {
    for (size_t a = 0; a < i; ++a) {
      ASSERT_FALSE(b.test(a));
    }
    ASSERT_TRUE(b.set(i, false));
    ASSERT_FALSE(b.set(i, false));
    for (size_t a = bits - 1; a > i; --a) {
      ASSERT_TRUE(b.test(a));
    }
  }
}

template <typename T>
void
test_seq_setFalse_get() {
  constexpr size_t bits = 1024;
  Bitset<bits, T> b;
  true_set(b);
  false_set(b);
}

TEST_F(BitsetTest, test_seq_setFalse_get_short) {
  test_seq_setFalse_get<uint16_t>();
}

TEST_F(BitsetTest, test_seq_setFalse_get_int) {
  test_seq_setFalse_get<uint32_t>();
}

TEST_F(BitsetTest, test_seq_setFalse_get_byte) {
  test_seq_setFalse_get<uint8_t>();
}

TEST_F(BitsetTest, test_seq_setFalse_get_long) {
  test_seq_setFalse_get<uint64_t>();
}

TEST_P(BitsetTest, test_set) {
  printf("test_set(%s)\n", GetParam() ? "true" : "false");
  Bitset<1024, uint8_t> b{GetParam()};
  std::size_t i = 0;
  ASSERT_EQ(GetParam(), b.test(i));
  bool val = !GetParam();
  ASSERT_TRUE(b.set(i, val));
  ASSERT_FALSE(b.set(i, val));
}

std::string
random_binary(size_t cnt) {
  std::mt19937 mt(0);
  std::uniform_int_distribution<int> dist(0, 1);
  std::string stream = "";
  for (size_t i = 0; i < cnt; ++i) {
    if (dist(mt)) {
      stream.append("1");
    } else {
      stream.append("0");
    }
  }
  return stream;
}

struct Timer {
public:
  const std::clock_t start;
  const std::string m_msg;

  explicit Timer(const std::string &msg)
      : start{std::clock()}
      , m_msg{msg} {
  }

  ~Timer() {
    std::clock_t stop = std::clock();
    cout << endl << m_msg << ": " << (stop - start) << "ns" << endl;
  }
};

template <typename F>
auto
time(const std::string &msg, F f) -> decltype(f()) {
  Timer t(msg);
  return f();
}

template <typename T>
void
test_init() {
  constexpr size_t bits(1024 * 80);
  std::string str = random_binary(bits);
  std::bitset<bits> init(str);
  using Bitset_t = Bitset<bits, T>;
  //    cout << endl << init << endl;

  // std::aligned_storage<sizeof(Bitset_t), alignof(Bitset_t)> braw;
  // auto &b = time(__PRETTY_FUNCTION__, [&braw, &init]() -> Bitset_t & { //
  //   Bitset_t *b = new (&braw) Bitset_t(init);
  //   return *b;
  // });
  Bitset_t b(init);
  //    cout << b << endl;
  for (size_t i = 0; i < init.size(); ++i) {
    // printf("%lu\n", i);
    ASSERT_EQ(init.test(i), b.test(i));
  }
}

TEST_F(BitsetTest, init_long) {
  test_init<uint64_t>();
}

TEST_F(BitsetTest, init_int) {
  test_init<uint32_t>();
}

TEST_F(BitsetTest, init_short) {
  test_init<uint16_t>();
}

TEST_F(BitsetTest, init_byte) {
  test_init<uint8_t>();
}

TEST_F(BitsetTest, init_set_fill) {
  constexpr size_t bits(1024 * 80);
  std::string str = random_binary(bits);
  std::bitset<bits> init(str);
  Bitset<bits, unsigned long> bb;
  time("init_set<unsigned long>", [&] {
    for (size_t i = 0; i < bits; ++i) {
      bb.set(i, init[i]);
    }
  });
}

template <typename T>
void
test_set_random(bool v) {
  constexpr size_t bits(1024);
  Bitset<bits, T> bb(!v);
  std::array<size_t, bits> in;
  {
    size_t val = 0;
    std::generate(in.begin(), in.end(), [&] { return val++; });
  }
  std::shuffle(in.begin(), in.end(), std::default_random_engine(0));
  std::unordered_set<size_t> present;
  for (const size_t &it : in) {
    ASSERT_TRUE(bb.set(it, v));
    ASSERT_TRUE(present.insert(it).second);
    for (size_t i = 0; i < bb.size(); ++i) {
      if (present.find(i) != present.end()) {
        if (v) {
          ASSERT_TRUE(bb.test(i));
        } else {
          ASSERT_FALSE(bb.test(i));
        }
      } else {
        if (v) {
          ASSERT_FALSE(bb.test(i));
        } else {
          ASSERT_TRUE(bb.test(i));
        }
      }
    }
  }
}

TEST_P(BitsetTest, test_long_random) {
  test_set_random<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_int_random) {
  test_set_random<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_short_random) {
  test_set_random<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_byte_random) {
  test_set_random<uint8_t>(GetParam());
}

template <typename T>
void
test_find(bool v) {
  constexpr size_t bits(1024);
  Bitset<bits, T> bb{!v};
  for (size_t i = 0; i < bits; ++i) {
    //        cout << "(" << v << ")" << i << endl;
    ASSERT_EQ(bits, bb.find_first(i, v));
    ASSERT_TRUE(bb.set(i, v));
    ASSERT_FALSE(bb.set(i, v));
    ASSERT_EQ(v, bb.test(i));
    ASSERT_EQ(i, bb.find_first(i, v));
  }
}

TEST_P(BitsetTest, test_findlong) {
  test_find<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_findint) {
  test_find<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_findshort) {
  test_find<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_findbyte) {
  test_find<uint8_t>(GetParam());
}

template <typename T>
void
test_find_reverse(bool v) {
  constexpr size_t bits(1024);
  Bitset<bits, T> bb{!v};
  for (size_t i = bb.size(); i-- > 0;) {
    ASSERT_TRUE(bb.set(i, v));
    ASSERT_EQ(v, bb.test(i));
    ASSERT_EQ(i, bb.find_first(v));
  }
}

TEST_P(BitsetTest, test_findlong_reverse) {
  test_find_reverse<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_findint_reverse) {
  test_find_reverse<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_findshort_reverse) {
  test_find_reverse<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_findbyte_reverse) {
  test_find_reverse<uint8_t>(GetParam());
}

template <typename T>
void
test_all_reverse(bool v) {
  constexpr size_t bits(1024);
  Bitset<bits, T> bb{!v};
  cout << endl;
  for (size_t i = bb.size(); i-- > 0;) {
    // cout << "(" << !v << ")" << i << endl;
    ASSERT_FALSE(bb.all(i, v));
    ASSERT_EQ(!v, bb.test(i));
    ASSERT_TRUE(bb.set(i, v));
    ASSERT_EQ(v, bb.test(i));
    ASSERT_TRUE(bb.all(i, v));
  }
}

TEST_P(BitsetTest, test_all_reverselong_reverse) {
  test_all_reverse<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_all_reverseint_reverse) {
  test_all_reverse<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_all_reverseshort_reverse) {
  test_all_reverse<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_all_reversebyte_reverse) {
  test_all_reverse<uint8_t>(GetParam());
}

template <typename T>
void
test_all_prefill(bool v) {
  constexpr size_t bits(1024);
  Bitset<bits, T> bb{!v};
  cout << endl;
  for (size_t i = 0; i < bits; ++i) {
    //    cout << i << endl;
    ASSERT_TRUE(bb.all(i, !v));
    ASSERT_FALSE(bb.all(i, v));
  }
}

TEST_P(BitsetTest, test_all_prefilllong) {
  test_all_prefill<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_all_prefillint) {
  test_all_prefill<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_all_prefillshort) {
  test_all_prefill<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_all_prefillbyte) {
  test_all_prefill<uint8_t>(GetParam());
}

template <typename T>
void
test_swap_first(bool v) {
  constexpr size_t bits(1024);
  Bitset<bits, T> bb{!v};
  for (size_t i = 0; i < bb.size(); ++i) {
    for (size_t a = i; a < bb.size(); ++a) {
      ASSERT_EQ(!v, bb.test(a));
    }
    ASSERT_EQ(i, bb.swap_first(0, v));
    for (size_t a = 0; a <= i; ++a) {
      ASSERT_EQ(v, bb.test(a));
    }
  }
}

TEST_P(BitsetTest, test_swap_firstlong_reverse) {
  test_swap_first<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_firstint_reverse) {
  test_swap_first<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_firstshort_reverse) {
  test_swap_first<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_firstbyte_reverse) {
  test_swap_first<uint8_t>(GetParam());
}

template <typename T, size_t S>
size_t
find_next_(size_t off, bool v, const Bitset<S, T> &bb) {
  for (size_t i = off; i < bb.size(); ++i) {
    if (bb.test(i) == v) {
      return i;
    }
  }
  return bb.size();
}

template <typename T>
void
test_swap_first_random(bool v) {
  constexpr size_t bits(1024);
  std::string str = random_binary(bits);
  std::bitset<bits> init(str);
  Bitset<bits, T> bb{init};
  size_t pos(0);
  // cout << endl << bb.to_string() << endl;
  while (true) {
    // auto posb = pos;
    pos = find_next_(pos, !v, bb);
    if (pos < bb.size()) {
      // printf("pos[%zu] = find_next_(pos[%zu], !v[%d], bb)\n", pos, posb, !v);

      size_t res = bb.swap_first(pos, v);
      ASSERT_EQ(pos, res);
    } else {
      break;
    }
  }
}

TEST_P(BitsetTest, test_swap_first_random_long) {
  test_swap_first_random<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_first_random_int) {
  test_swap_first_random<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_first_random_short) {
  test_swap_first_random<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_first_random_byte) {
  test_swap_first_random<uint8_t>(GetParam());
}

template <typename T>
void
test_swap_limit_length(bool v) {
  constexpr size_t bits(1024);
  Bitset<bits, T> bb{v};
  ASSERT_EQ(bb.swap_first(v, 0), bb.npos);

  for (size_t i = 0; i < bits; ++i) {
    ASSERT_EQ(bb.swap_first(!v, i + 1), i);
    ASSERT_EQ(bb.swap_first(!v, i + 1), bb.npos);
  }
}
TEST_P(BitsetTest, test_swap_limit_long) {
  test_swap_limit_length<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_limit_int) {
  test_swap_limit_length<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_limit_short) {
  test_swap_limit_length<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_limit_byte) {
  test_swap_limit_length<uint8_t>(GetParam());
}

template <typename T>
void
test_swap_window(bool v) {
  constexpr size_t bits(1024);
  Bitset<bits, T> bb{v};
  ASSERT_EQ(bb.swap_first(v, 0), bb.npos);

  for (size_t i = 0; i < bits; ++i) {
    ASSERT_EQ(bb.swap_first(i, !v, i + 1), i);
    ASSERT_EQ(bb.swap_first(i, !v, i + 1), bb.npos);
  }
}

TEST_P(BitsetTest, test_swap_window_long) {
  test_swap_window<uint64_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_window_int) {
  test_swap_window<uint32_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_window_short) {
  test_swap_window<uint16_t>(GetParam());
}

TEST_P(BitsetTest, test_swap_window_byte) {
  test_swap_window<uint8_t>(GetParam());
}

std::string
reverse(const std::string &s) {
  std::string res;
  res.reserve(s.size());
  for (size_t i = s.size(); i-- > 0;) {
    res.push_back(s[i]);
  }
  return res;
}

template <typename T>
void
test_to_string() {
  constexpr size_t bits(1024);
  std::string str = random_binary(bits);
  std::bitset<bits> init(str);
  Bitset<bits, T> bb{init};
  ASSERT_EQ(init.to_string(), reverse(bb.to_string()));
}

TEST_F(BitsetTest, test_to_stringlong) {
  test_to_string<uint64_t>();
}

TEST_F(BitsetTest, test_to_stringint) {
  test_to_string<uint32_t>();
}

TEST_F(BitsetTest, test_to_stringshort) {
  test_to_string<uint16_t>();
}

TEST_F(BitsetTest, test_to_stringbyte) {
  test_to_string<uint8_t>();
}

template <typename T, size_t bits>
void
test_threaded_find_fist() {
}
