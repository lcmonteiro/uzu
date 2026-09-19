#include <array>
#include <functional>
#include <iostream>
#include <vector>

#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/functional.hpp"
#include "uzu/foundation/dual/operations/cos.hpp"
#include "uzu/foundation/dual/operations/divides.hpp"
#include "uzu/foundation/dual/operations/exp.hpp"
#include "uzu/foundation/dual/operations/log.hpp"
#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"
#include "uzu/foundation/dual/operations/pow.hpp"
#include "uzu/foundation/dual/operations/sin.hpp"
#include "uzu/foundation/dual/operations/sqrt.hpp"
#include "uzu/foundation/dual/print.hpp"

template <std::size_t D>
using number = dual::number<double, D>;

template <typename... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
template <typename... Ts>
overloads(Ts...) -> overloads<Ts...>;

template <typename T, typename = void>
struct is_tuple_like : std::false_type {};

template <typename T>
struct is_tuple_like<T, std::void_t<decltype(std::tuple_size_v<T>)>> : std::true_type {};

int main() {
  auto b = dual::make_array<0>(1.0, 2.0, 3.);
  auto a = std::array{1.0, 2.0, 4.0};
  auto n = dual::number<double, 3>{2.0};
  auto d = std::cos(std::sin(std::log(std::exp(a / b + a))));
  // Instantiated but not printed: the output of this program is compared byte for byte
  // against earlier runs, so what it prints stays fixed.
  [[maybe_unused]] auto e = std::cos(std::sin(n)) * 4.0;
  auto f = dual::array(std::tie(n, n));
  // dual::product used to be the multiplicative twin of summation; in the
  // current library it is the cartesian product of containers, so fold the
  // elements with dual::apply instead.
  auto g = dual::apply([](const auto &...e) { return ((e + 1.0) * ...); }, b);

  [[maybe_unused]] auto h = dual::make_array<3>(a);
  dual::apply([](auto) {}, n);
  dual::print(std::pow(b, 0.5));
  dual::print(std::sqrt(b));
  dual::print(d);
  dual::print(f);
  dual::print(g);

  // dual::plus_transform::enable_t<dual::array<>, int> z;
  // using T1 = dual::array<>;
  // using T2 = int;
  // decltype(std::tuple_size_v<int>) h{};
  //	dual::print(is_tuple_like<int>::value);
  // dual::print(dual::is_number_like_v<decltype(n)>
  // or
  //	(dual::is_array_v<T2> and dual::is_array_like_v<T1>)
  //	);
  //	print(d);
  /*
  auto f = std::cos(
         std::pow(
           std::get<0>(d),
           std::log(std::get<1>(d))) +
         3.0 * std::exp(std::get<1>(d) - std::get<0>(d))) /
       std::sin(std::get<2>(d));


  number<0> x{2};
  number<1> y{3};
  number<2> z{5};
  auto f = std::cos(
         std::pow(
           x,
           std::log(y)) +
         3.0 * std::exp(y - x)) /
       std::sin(z);

  std::cout << "f     = " << f.value() << std::endl;
  std::cout << "df/dx = " << f.dvalue<0>() << std::endl;
  std::cout << "df/dy = " << f.dvalue<1>() << std::endl;
  std::cout << "df/dz = " << f.dvalue<2>() << std::endl;
*/
  return 0;
}
