#include "blind.hpp"

#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <deque>
#include <string>
#include <sstream>

#include <cassert>

std::ostream& print(std::ostream& os) {
  return os;
}
template<typename H, typename... Rest>
std::ostream& print(std::ostream& os, H&& h, Rest&&... rest) {
  os << std::forward<H>(h);
  return print(os, std::forward<Rest>(rest)...);
}

int main(const int argc, const char** argv) {
  using namespace std;
  using namespace com::masaers::blind;
  using namespace std::placeholders;

  {
    const auto rsort = blind(BLIND_FUNC(sort), _1, _2, [](auto&& a, auto&& b) { return a > b; });
    vector<int> vec{ 1, 2, 3, 4 };
    rsort(begin(vec), end(vec));
    {
      auto it = begin(vec);
      assert(*it++ == 4);
      assert(*it++ == 3);
      assert(*it++ == 2);
      assert(*it++ == 1);
      assert(it == end(vec));
    }

    deque<string> dq{ "a", "b", "c", "d" };
    rsort(begin(dq), end(dq));
    {
      auto it = begin(dq);
      assert(*it++ == "d");
      assert(*it++ == "c");
      assert(*it++ == "b");
      assert(*it++ == "a");
      assert(it == end(dq));
    }
  }
  {
    ostringstream os;
    const auto p = blind(BLIND_FUNC(print), ref(os));
    p(1, 1.0, '1', "1");
    assert(os.str() == "1111");
  }
  {
    vector<int> vec{ 1, 2, 3, 4 };
    auto sort_vec = blind(BLIND_FUNC(sort), begin(vec), end(vec));
    sort_vec([](auto&& a, auto&& b) { return a > b; });
    {
      auto it = begin(vec);
      assert(*it++ == 4);
      assert(*it++ == 3);
      assert(*it++ == 2);
      assert(*it++ == 1);
      assert(it == end(vec));
    }
    sort_vec();
    {
      auto it = begin(vec);
      assert(*it++ == 1);
      assert(*it++ == 2);
      assert(*it++ == 3);
      assert(*it++ == 4);
      assert(it == end(vec));
    }
  }
  return EXIT_SUCCESS;
}

