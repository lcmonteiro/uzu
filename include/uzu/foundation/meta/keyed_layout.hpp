/// ===============================================================================================
/// @file
///
/// @brief A compile-time layout of keyed blocks, laid end to end in one contiguous index space.
///
/// Given entries that each carry a key and a width, the layout gives every entry one block of
/// indices, in declaration order, and answers the two questions anything indexing into that space
/// asks: which entry a key names, and where that entry's block starts. Everything is a constant
/// expression, so the answers can be template arguments.
///
/// Nothing here knows what the entries are. The factor graph lays its nodes out with it - a
/// node's key picks its block, its dimension sizes it - but any keyed, fixed-width set of things
/// sharing one index space can use it the same way.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_META_KEYED_LAYOUT_HPP
#define UZU_FOUNDATION_META_KEYED_LAYOUT_HPP

#include <array>
#include <cstddef>
#include <tuple>

namespace uzu {
namespace meta {

/// @brief Keyed blocks laid end to end: block `i` has `Entries[i]::width` indices and starts
/// where block `i - 1` ends.
///
/// The layout reports whether its keys are distinct rather than asserting it, so that whoever
/// uses it can reject a repeat in their own words - "two nodes share a key" means more at the
/// point of use than "two entries do". Lookups take the first match, so a repeated key would
/// otherwise be silent: the second entry's block would sit in the layout with no key reaching it.
///
/// @tparam Entries Each supplies `static constexpr std::size_t key` and `width`.
template <class... Entries>
struct keyed_layout {
  /// @brief How many entries, and so how many blocks.
  static constexpr std::size_t size = sizeof...(Entries);

  /// @brief Each entry's key, in declaration order.
  static constexpr std::array<std::size_t, size> keys{Entries::key...};

  /// @brief Each entry's width, in declaration order.
  static constexpr std::array<std::size_t, size> widths{Entries::width...};

  /// @brief How many indices the whole layout spans: every block, and nothing between them.
  static constexpr std::size_t width = (Entries::width + ... + 0);

  /// @brief The entry at position @p I.
  template <std::size_t I>
  using entry_at = std::tuple_element_t<I, std::tuple<Entries...>>;

  /// @brief Which entry carries @p key, or #size if none does.
  ///
  /// The one scan over the keys: #contains and #offset_of go through here rather than repeating it.
  static constexpr auto index_of(std::size_t key) -> std::size_t {
    for (std::size_t i = 0; i < size; ++i) {
      if (keys[i] == key) {
        return i;
      }
    }
    return size;
  }

  /// @brief Whether any entry carries @p key.
  static constexpr auto contains(std::size_t key) -> bool {
    return index_of(key) != size;
  }

  /// @brief Where the block of the entry carrying @p key starts, or #width if none does.
  static constexpr auto offset_of(std::size_t key) -> std::size_t {
    const std::size_t index = index_of(key);

    std::size_t offset = 0;
    for (std::size_t i = 0; i < index; ++i) {
      offset += widths[i];
    }
    return offset;
  }

  /// @brief Whether no two entries carry the same key.
  static constexpr bool distinct_keys = [] {
    for (std::size_t i = 0; i < size; ++i) {
      for (std::size_t j = i + 1; j < size; ++j) {
        if (keys[i] == keys[j]) {
          return false;
        }
      }
    }
    return true;
  }();
};

}  // namespace meta
}  // namespace uzu

#endif  // UZU_FOUNDATION_META_KEYED_LAYOUT_HPP
