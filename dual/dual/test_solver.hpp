#include <array>
#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>

auto print_number = [](auto &name, auto &number) {
	std::cout << name << " = " << number << std::endl;
};

auto print_matrix = [](auto &name, auto &matrix) {
	std::cout << name << " = "
			  << "[";
	for (auto vector : matrix)
	{
		std::cout << std::endl;
		for (auto val : vector)
			std::cout << " " << val;
	}
	std::cout << std::endl
			  << "]" << std::endl;
};

struct Config
{
	static constexpr auto kTolerance = 0.1;
};

template <typename Type, class Config = Config>
struct Space
{
	static inline auto One() { return Type{1}; }
	static inline auto Zero() { return Type{0}; }
	static inline auto IsOne(Type a)
	{
		return (
			(Type{1 + Config::kTolerance} >= a) and
			(Type{1 - Config::kTolerance} <= a));
	}
	static inline auto IsZero(Type a)
	{
		return (
			(Type{0 + Config::kTolerance} >= a) and
			(Type{0 - Config::kTolerance} <= a));
	}
	static inline auto Div(Type a, Type b) { return a / b; }
	static inline auto Mul(Type a, Type b) { return a * b; }
	static inline auto Sub(Type a, Type b) { return a - b; }
	static inline auto Add(Type a, Type b) { return a + b; }
};

template <typename Vector, typename Func>
void apply(Vector &out, const Vector &in, Func op, size_t offset)
{
	auto p0 = std::next(std::begin(out), offset);
	auto p1 = std::next(std::begin(in), offset);
	auto pe = std::end(out);
	std::transform(p0, pe, p1, p0, op);
}
template <typename Vector, typename Func>
void apply(Vector &out, const Vector &in, Func op)
{
	auto p0 = std::begin(out);
	auto p1 = std::begin(in);
	auto pe = std::end(out);
	std::transform(p0, pe, p1, p0, op);
}
template <typename Vector, typename Type, typename Func>
void apply(Vector &out, Type in, Func op, size_t offset)
{
	auto p0 = std::next(std::begin(out), offset);
	auto pe = std::end(out);
	std::transform(p0, pe, p0, [&op, &in](auto &a) { return op(a, in); });
}
template <typename Vector, typename Type, typename Func>
void apply(Vector &out, Type in, Func op)
{
	auto p0 = std::begin(out);
	auto pe = std::end(out);
	std::transform(p0, pe, p0, [&op, &in](auto &a) { return op(a, in); });
}

template <typename Space, typename MatrixA, typename MatrixB>
inline bool forward_prepare(size_t index, MatrixA &coef, MatrixB &data)
{
	if (!Space::IsZero(coef[index][index]))
	{
		return true;
	}
	for (auto i = index + 1, ii = data.size(); i < ii; ++i)
	{
		if (!Space::IsZero(coef[i][index]))
		{
			std::swap(coef[index], coef[i]);
			std::swap(data[index], data[i]);
			return true;
		}
	}
	return false;
}

template <typename Space, typename MatrixA, typename MatrixB>
inline void forward_elimination(size_t index, MatrixA &coef, MatrixB &data)
{
	auto coef_reference = coef[index][index];
	for (auto i = size_t{index + 1}; i < data.size(); ++i)
	{
		auto coef_to_reset = coef[i][index];
		if (Space::IsZero(coef_to_reset))
		{
			continue;
		}
		auto factor = Space::Div(coef_reference, coef_to_reset);
		if (Space::IsOne(factor))
		{
			apply(coef[i], coef[index], Space::Sub, index);
			apply(data[i], data[index], Space::Sub);
			continue;
		}
		apply(coef[i], factor, Space::Mul, index);
		apply(data[i], factor, Space::Mul);
		apply(coef[i], coef[index], Space::Sub, index);
		apply(data[i], data[index], Space::Sub);
	}
}

template <typename Space, typename MatrixA, typename MatrixB>
inline void backward_elimination(size_t index, MatrixA &coef, MatrixB &data)
{
	for (auto i = size_t{0}; i < index; ++i)
	{
		auto coef_reference = coef[index][index];
		auto coef_to_reset = coef[i][index];
		if (Space::IsZero(coef_to_reset))
		{
			continue;
		}
		auto factor = Space::Div(coef_to_reset, coef_reference);
		if (Space::IsOne(factor))
		{
			apply(coef[i], coef[index], Space::Sub, index);
			apply(data[i], data[index], Space::Sub);
			continue;
		}
		apply(coef[index], factor, Space::Mul, index);
		apply(data[index], factor, Space::Mul);
		apply(coef[i], coef[index], Space::Sub, index);
		apply(data[i], data[index], Space::Sub);
	}
}

template <typename Space, typename MatrixA, typename MatrixB>
static inline void normalization(size_t index, MatrixA &coef, MatrixB &data)
{
	auto factor = Space::Div(Space::One(), coef[index][index]);
	if (Space::IsOne(factor))
	{
		return;
	}
	apply(coef[index], factor, Space::Mul, index);
	apply(data[index], factor, Space::Mul);
}

template <typename Space, typename MatrixA, typename MatrixB>
inline size_t solve(size_t max_size, MatrixA &coef, MatrixB &data)
{
	auto n = size_t{0};
	for (; n < max_size && n < data.size(); ++n)
	{
		if (not forward_prepare<Space>(n, coef, data))
		{
			break;
		}
		forward_elimination<Space>(n, coef, data);
	}
	for (auto i = size_t{1}; i < n; ++i)
	{
		backward_elimination<Space>(i, coef, data);
	}
	for (auto i = size_t{0}; i < n; ++i)
	{
		normalization<Space>(i, coef, data);
	}
	return n;
}

int main()
{
	auto matrix_a = std::array{
		std::array{1.0, 2.0, -1.0},
		std::array{2.0, 1.0, 2.0},
		std::array{-1.0, 2.0, 1.0}};
	/*
    auto matrix_a = std::array{
        std::array{ 3.0, 4.0},
        std::array{ 6.0, 8.0}
    };*/
	auto matrix_b = std::array{
		std::array{10.0},
		std::array{5.0},
		std::array{6.0},
	};

	/*
    auto matrix_b = std::array{
        std::array{1.0, 0.0},
        std::array{0.0, 1.0},
    };*/
	solve<Space<double>>(3, matrix_a, matrix_b);

	print_matrix("a", matrix_a);
	print_matrix("b", matrix_b);

	return 0;
}