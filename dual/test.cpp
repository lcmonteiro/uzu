#include <iostream>
#include <array>
#include <vector>
#include <functional>

#include "dual/array.hpp"
#include "dual/number.hpp"
#include "dual/operations/plus.hpp"
#include "dual/operations/minus.hpp"
#include "dual/operations/multiplies.hpp"
#include "dual/operations/divides.hpp"
#include "dual/operations/exp.hpp"
#include "dual/operations/pow.hpp"
#include "dual/operations/log.hpp"
#include "dual/operations/sin.hpp"
#include "dual/operations/cos.hpp"

template <std::size_t D>
using number = dual::number<double, D>;

template <typename... Ts>
struct overloads : Ts...
{
	using Ts::operator()...;
};
template <typename... Ts>
overloads(Ts...)->overloads<Ts...>;

int main()
{
	auto b = dual::make_array<0, 3>(1.0);
	auto a = std::array{1.0, 2.0, 4.0};
	auto d = b + a;
	auto f = std::cos(
				 std::pow(
					 std::get<0>(d),
					 std::log(std::get<1>(d))) +
				 3.0 * std::exp(std::get<1>(d) - std::get<0>(d))) /
			 std::sin(std::get<2>(d));
/*

	number<0> x{2};
	number<1> y{3};
	number<2> z{5};
	auto f = std::cos(
				 std::pow(
					 x,
					 std::log(y)) +
				 3.0 * std::exp(y - x)) /
			 std::sin(z);
*/
	std::cout << "f     = " << f.value() << std::endl;
	std::cout << "df/dx = " << f.dvalue<0>() << std::endl;
	std::cout << "df/dy = " << f.dvalue<1>() << std::endl;
	std::cout << "df/dz = " << f.dvalue<2>() << std::endl;

	return 0;
}
