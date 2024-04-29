#include <algorithm>
#include <catch2/catch_all.hpp>
#include <iostream>

static constexpr double EPSILON = 0.00000000001;
inline bool is_equal(double l, double r) { return std::fabs(l - r) < EPSILON; }
struct double_less_eq
{
  bool operator()(const double& l, const double& r) const { return is_equal(l, r) || l < r; }
};

struct double_greater_eq
{
  bool operator()(double l, double r) const { return is_equal(l, r) || l > r; }
};

TEST_CASE("sorted")
{
  {
    std::vector<int> v{1, 1};
    CHECK(true == std::is_sorted(begin(v), end(v)));
    CHECK(end(v) != std::adjacent_find(begin(v), end(v)));
  }

  {
    std::vector<int> v{1, 2, 5};
    CHECK(end(v) == std::adjacent_find(begin(v), end(v)));
  }

  {
    std::vector<double> v{1.3, 2.3, 5.5};
    CHECK(end(v) == std::adjacent_find(begin(v), end(v), double_greater_eq{}));
  }

  {
    std::vector<double> v{1.3, 2.3, 5.5, 5.5};
    CHECK(end(v) != std::adjacent_find(begin(v), end(v), double_greater_eq{}));
  }
}

int main(int argc, char** argv) { return Catch::Session().run(argc, argv); }
