#include <array>
#include <functional>
#include <iostream>
#include <vector>

#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/number.hpp"
#include "uzu/foundation/dual/operations/cos.hpp"
#include "uzu/foundation/dual/operations/divides.hpp"
#include "uzu/foundation/dual/operations/exp.hpp"
#include "uzu/foundation/dual/operations/log.hpp"
#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"
#include "uzu/foundation/dual/operations/pow.hpp"
#include "uzu/foundation/dual/operations/sin.hpp"

template <std::size_t D>
using number = dual::number<double, D>;

template <typename... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
template <typename... Ts>
overloads(Ts...) -> overloads<Ts...>;

int main() {
  auto b = dual::make_array<0, 3>(1.0);
  auto a = std::array{1.0, 2.0, 4.0};
  auto d = b + a;
  auto f = cos(pow(get<0>(d), log(get<1>(d))) + 3.0 * exp(get<1>(d) - get<0>(d))) / sin(get<2>(d));
  /*

    number<0> x{2};
    number<1> y{3};
    number<2> z{5};
    auto f = cos(
           pow(
             x,
             log(y)) +
           3.0 * exp(y - x)) /
         sin(z);
  */
  std::cout << "f     = " << f.value() << std::endl;
  std::cout << "df/dx = " << f.dvalue<0>() << std::endl;
  std::cout << "df/dy = " << f.dvalue<1>() << std::endl;
  std::cout << "df/dz = " << f.dvalue<2>() << std::endl;

  return 0;
}
