/// ===============================================================================================
/// @file
///
/// @brief The edge base: a constraint between nodes, scored as an error.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_EDGE_HPP
#define UZU_OPTIMIZATION_GRAPH_EDGE_HPP

#include <array>
#include <cstddef>
#include <tuple>

#include "uzu/helpers/keywords.hpp"

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
/// `init` gives the measurement, the way it gives a node its estimate: an edge is a measurement
/// and the sigmas it is weighed with, so both are stated where the edge is built rather than one
/// at construction and the other through a setter afterwards. The setter stays, because a
/// measurement that arrives later - read from a sensor, or swept over in a loop - has to go
/// somewhere; what it is no longer needed for is saying what an edge *is*.
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

  /// @brief Builds an edge from `init` and `sigma`, in either order.
  ///
  /// `init` is the measurement and takes one value per coordinate of `measurement_type`; `sigma`
  /// takes one per residual, or a single one broadcast across them all. Either may be left out:
  /// the measurement then starts zeroed, and the sigmas at one.
  ///
  /// An edge with nothing to compare against declares `std::array<double, 0>`, and giving *that*
  /// an `init` is a compile error rather than a value quietly dropped - there is no coordinate
  /// for it to land in.
  template <impl::keyword_argument... Args>
  explicit constexpr edge(const Args &...args)
      : sigma_{impl::find_all<impl::sigma_tag>(filled(value_type{1}), args...)},
        measurement_{measured(args...)} {
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
  /// @brief The measurement `init` asks for, or a zeroed one.
  ///
  /// An edge with nothing to compare against is the case worth intercepting. The keyword's own
  /// width check would catch an `init` given to one, but it reports a count rather than the thing
  /// that is wrong, and a second diagnostic follows it out of the zero-width array. Answering
  /// here, before the keyword is ever consulted, leaves one message that names the mistake.
  template <class... Args>
  static constexpr auto measured(const Args &...args) -> measurement_type {
    if constexpr (std::tuple_size_v<measurement_type> == 0) {
      static_assert(
          !(impl::tagged<impl::init_tag, Args> || ...),
          "this edge declares no measurement, so there is nothing for `init` to set: give it a "
          "measurement_type with coordinates, or drop the keyword");
      return measurement_type{};
    } else {
      return impl::find<impl::init_tag>(measurement_type{}, args...);
    }
  }

  /// @brief Sigmas with every entry set to the same value.
  static constexpr auto filled(value_type value) -> point {
    point out{};
    for (std::size_t i = 0; i < dimension; ++i) {
      out[i] = value;
    }
    return out;
  }

  // In the order the constructor's member-initializer list writes them, so the two agree and
  // -Wreorder has nothing to say.
  point sigma_{};
  measurement_type measurement_{};
};

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_EDGE_HPP
