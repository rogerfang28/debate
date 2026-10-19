#pragma once
// Fixed iteration orders for whatever reaches the page.
//
// Every click re-renders the WHOLE page, so two responses for the same state
// must come out identical: same components, same order, same ids. Protobuf
// maps (claims_by_id, links_by_id, user_statuses) and the std::unordered_*
// containers iterate in an unspecified order -- protobuf seeds its map hash
// per instance -- so a list built straight from one can shuffle between two
// requests, and React then moves DOM nodes and the page twitches.
//
// The rule: anything iterated to BUILD a list on the page goes through one
// of these first. Claims and relations by id ascending (which is the order
// they were made), people by name, case-insensitively. Lookups (find, count)
// and order-free reductions (a minimum, a count) need none of this.
//
// Header-only on purpose: no new translation unit for either CMake list.

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

namespace ordered {

// People by name: case-insensitive, then exact as a tiebreak so "paul" and
// "Paul" still have one fixed order.
inline bool NameLess(const std::string& a, const std::string& b) {
    const size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        const int ca = std::tolower(static_cast<unsigned char>(a[i]));
        const int cb = std::tolower(static_cast<unsigned char>(b[i]));
        if (ca != cb) return ca < cb;
    }
    if (a.size() != b.size()) return a.size() < b.size();
    return a < b;
}

struct NameLessFn {
    bool operator()(const std::string& a, const std::string& b) const { return NameLess(a, b); }
};

// A set of names that iterates in display order.
using NameSet = std::set<std::string, NameLessFn>;

// The keys of any map, sorted: numbers ascending, names by NameLess.
template <class Map>
std::vector<typename Map::key_type> SortedKeys(const Map& m) {
    using Key = typename Map::key_type;
    std::vector<Key> keys;
    keys.reserve(m.size());
    for (const auto& entry : m) keys.push_back(entry.first);
    if constexpr (std::is_same_v<Key, std::string>) {
        std::sort(keys.begin(), keys.end(), NameLess);
    } else {
        std::sort(keys.begin(), keys.end());
    }
    return keys;
}

// The entries of any map, by key (see SortedKeys), as pointers into it:
//   for (const auto* e : ordered::Entries(claim.user_statuses())) e->first ...
template <class Map>
std::vector<const typename Map::value_type*> Entries(const Map& m) {
    using Key = typename Map::key_type;
    std::vector<const typename Map::value_type*> out;
    out.reserve(m.size());
    for (const auto& entry : m) out.push_back(&entry);
    std::sort(out.begin(), out.end(), [](const auto* a, const auto* b) {
        if constexpr (std::is_same_v<Key, std::string>) {
            return NameLess(a->first, b->first);
        } else {
            return a->first < b->first;
        }
    });
    return out;
}

// A debate::Collection's claims, by id ascending.
template <class Collection>
std::vector<int> SortedClaimIds(const Collection& c) {
    return SortedKeys(c.claims_by_id());
}

// A debate::Collection's relations, by id ascending.
template <class Collection>
std::vector<int> SortedLinkIds(const Collection& c) {
    return SortedKeys(c.links_by_id());
}

// The names keying a map (user_statuses and the like), in display order.
template <class Map>
std::vector<std::string> SortedNames(const Map& m) {
    return SortedKeys(m);
}

}  // namespace ordered
