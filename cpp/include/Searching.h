/*******************************************************************************
 * Searching.h
 * Binary Search (on data pre-sorted by ReportManager / DroneManager) and
 * Linear Search (fallback / unsorted small collections) used for fast record
 * lookup across the system.
 ******************************************************************************/
#ifndef SEARCHING_H
#define SEARCHING_H

#include <vector>
#include <functional>

namespace dsa {

// Binary search over a vector sorted ascending by keyOf(element).
// Returns index of match, or -1 if not found.
template <typename T, typename KeyT>
int binarySearch(const std::vector<T>& arr, const KeyT& target,
                  const std::function<KeyT(const T&)>& keyOf) {
    int lo = 0, hi = static_cast<int>(arr.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        KeyT midKey = keyOf(arr[mid]);
        if (midKey == target) return mid;
        if (midKey < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

// Linear search fallback - O(n), used for small / unsorted collections.
template <typename T, typename KeyT>
int linearSearch(const std::vector<T>& arr, const KeyT& target,
                  const std::function<KeyT(const T&)>& keyOf) {
    for (size_t i = 0; i < arr.size(); ++i) {
        if (keyOf(arr[i]) == target) return static_cast<int>(i);
    }
    return -1;
}

} // namespace dsa

#endif // SEARCHING_H
