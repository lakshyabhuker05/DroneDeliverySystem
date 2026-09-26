/*******************************************************************************
 * Sorting.h
 * Generic Merge Sort and Quick Sort implementations used by ReportManager to
 * sort deliveries (by date, status, weight, distance...) before generating
 * daily / weekly / monthly reports.
 ******************************************************************************/
#ifndef SORTING_H
#define SORTING_H

#include <vector>
#include <functional>

namespace dsa {

// ------------------------- MERGE SORT (stable, O(n log n)) -----------------
template <typename T>
void mergeHelper(std::vector<T>& arr, int l, int m, int r, const std::function<bool(const T&, const T&)>& less) {
    std::vector<T> left(arr.begin() + l, arr.begin() + m + 1);
    std::vector<T> right(arr.begin() + m + 1, arr.begin() + r + 1);
    size_t i = 0, j = 0;
    int k = l;
    while (i < left.size() && j < right.size()) {
        if (less(right[j], left[i])) arr[k++] = right[j++];
        else arr[k++] = left[i++];
    }
    while (i < left.size()) arr[k++] = left[i++];
    while (j < right.size()) arr[k++] = right[j++];
}

template <typename T>
void mergeSortRec(std::vector<T>& arr, int l, int r, const std::function<bool(const T&, const T&)>& less) {
    if (l >= r) return;
    int m = l + (r - l) / 2;
    mergeSortRec(arr, l, m, less);
    mergeSortRec(arr, m + 1, r, less);
    mergeHelper(arr, l, m, r, less);
}

template <typename T>
void mergeSort(std::vector<T>& arr, const std::function<bool(const T&, const T&)>& less) {
    if (arr.size() < 2) return;
    mergeSortRec(arr, 0, static_cast<int>(arr.size()) - 1, less);
}

// -------------------------- QUICK SORT (in-place, avg O(n log n)) ----------
template <typename T>
int partitionHelper(std::vector<T>& arr, int low, int high, const std::function<bool(const T&, const T&)>& less) {
    T pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (less(arr[j], pivot)) {
            i++;
            std::swap(arr[i], arr[j]);
        }
    }
    std::swap(arr[i + 1], arr[high]);
    return i + 1;
}

template <typename T>
void quickSortRec(std::vector<T>& arr, int low, int high, const std::function<bool(const T&, const T&)>& less) {
    if (low < high) {
        int p = partitionHelper(arr, low, high, less);
        quickSortRec(arr, low, p - 1, less);
        quickSortRec(arr, p + 1, high, less);
    }
}

template <typename T>
void quickSort(std::vector<T>& arr, const std::function<bool(const T&, const T&)>& less) {
    if (arr.size() < 2) return;
    quickSortRec(arr, 0, static_cast<int>(arr.size()) - 1, less);
}

} // namespace dsa

#endif // SORTING_H
