/// @file edge.hpp
/// @brief The edge base: a constraint between nodes, scored as an error.

#pragma once

#include <array>
#include <cstddef>

#include "uzu/keywords.hpp"

namespace uzu {

/// @brief Base for an edge scoring one constraint.
///
/// The derived class supplies `error`, taking one estimate per node it is linked to and returning
/// `Dimension` residuals. It is a template because the graph hands it duals, and which dual
/// indices arrive depends on which nodes the edge links.
///
/// The error's width is declared rather than deduced from what `error` returns, which buys the
/// check that the two agree: an edge that returns the wrong number of residuals is a compile
/// error at the graph, naming the edge, instead of a sigma quietly zipped against the wrong row.
///
/// Sigma is one per residual, so `sigma = {2.0, 3.0}` sets them apart and `sigma = {2.0}` is
/// broadcast across them all. That is a property of the keyword now rather than of the type, so
/// an edge whose dimensions are alike is written the short way without declaring anything.
///
/// @tparam Derived The derived edge type.
/// @tparam Measurement What the edge is scored against. Named `measurement_type` inside, and
/// supplies the scalar as its own `value_type` - which is what this class then calls `value_type`
/// too. An edge with nothing to compare to declares `std::array<double, 0>`.
/// @tparam Dimension How many residuals `error` returns.
template <class Derived, class Measurement, auto Dimension>
class edge {
 public:
  /// @brief What the edge is measured against.
  using measurement_type = Measurement;

  /// @brief The scalar the measurement is made of, taken from the measurement itself.
  using value_type = typename Measurement::value_type;

  /// @brief The sigmas, one per residual.
  using point = std::array<value_type, Dimension>;

  /// @brief How many residuals `error` returns.
  static constexpr std::size_t dimension = Dimension;

  /// @brief Builds an edge from `sigma`, one value per residual or one for all.
  template <impl::keyword_argument... Args>
  explicit constexpr edge(const Args &...args)
      : sigma_{impl::find_all<impl::sigma_tag>(filled(value_type{1}), args...)} {
  }

  /// @brief The sigmas, laid out one per residual and ready to be zipped against one.
  constexpr auto sigma() const -> const point & {
    return sigma_;
  }

  /// @brief The measurement this edge is scored against.
  constexpr auto measurement() const -> const measurement_type & {
    return measurement_;
  }

  /// @brief Sets the measurement.
  constexpr auto measurement(const measurement_type &value) -> void {
    measurement_ = value;
  }

 private:
  /// @brief Sigmas with every entry set to the same value.
  static constexpr auto filled(value_type value) -> point {
    point out{};
    for (std::size_t i = 0; i < dimension; ++i) {
      out[i] = value;
    }
    return out;
  }

  point sigma_{};
  measurement_type measurement_{};
};

}  // namespace uzu
