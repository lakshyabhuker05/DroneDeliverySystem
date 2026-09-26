/*******************************************************************************
 * DataStructures.h
 * -----------------------------------------------------------------------------
 * Hand-written, from-scratch implementations of the core Data Structures used
 * throughout the Drone Delivery Management System.
 *
 * NOTE: These are deliberately implemented manually (not simply typedef'd to
 * STL containers) because the whole point of this project is to demonstrate
 * DSA fundamentals. Internally a couple of helper STL containers (std::vector)
 * are used purely as dynamic raw-memory backing where re-implementing dynamic
 * array growth would add no educational value; every *behavioural* structure
 * (linking, hashing, heap property, tree property, FIFO/LIFO order) is custom.
 ******************************************************************************/
#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include <iostream>

/* =============================== 1. QUEUE ================================
 * Singly linked-list based FIFO queue.
 * USED FOR: incoming delivery requests (first-come-first-served booking).
 * ========================================================================= */
template <typename T>
class LinkedQueue {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& d) : data(d), next(nullptr) {}
    };
    Node* frontPtr;
    Node* rearPtr;
    size_t count;

public:
    LinkedQueue() : frontPtr(nullptr), rearPtr(nullptr), count(0) {}
    ~LinkedQueue() { clear(); }

    void enqueue(const T& value) {
        Node* node = new Node(value);
        if (rearPtr == nullptr) {
            frontPtr = rearPtr = node;
        } else {
            rearPtr->next = node;
            rearPtr = node;
        }
        count++;
    }

    bool dequeue(T& out) {
        if (frontPtr == nullptr) return false;
        Node* temp = frontPtr;
        out = temp->data;
        frontPtr = frontPtr->next;
        if (frontPtr == nullptr) rearPtr = nullptr;
        delete temp;
        count--;
        return true;
    }

    bool peek(T& out) const {
        if (frontPtr == nullptr) return false;
        out = frontPtr->data;
        return true;
    }

    bool isEmpty() const { return frontPtr == nullptr; }
    size_t size() const { return count; }

    void clear() {
        T tmp;
        while (dequeue(tmp)) {}
    }

    // Returns a snapshot (front -> rear) without removing elements.
    std::vector<T> toVector() const {
        std::vector<T> out;
        Node* cur = frontPtr;
        while (cur) { out.push_back(cur->data); cur = cur->next; }
        return out;
    }
};

/* ============================ 2. STACK (LIFO) ==============================
 * Singly linked-list based Stack.
 * USED FOR: Undo operations (undo last cancel / undo last assignment).
 * ========================================================================= */
template <typename T>
class LinkedStack {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& d) : data(d), next(nullptr) {}
    };
    Node* topPtr;
    size_t count;

public:
    LinkedStack() : topPtr(nullptr), count(0) {}
    ~LinkedStack() { clear(); }

    void push(const T& value) {
        Node* node = new Node(value);
        node->next = topPtr;
        topPtr = node;
        count++;
    }

    bool pop(T& out) {
        if (topPtr == nullptr) return false;
        Node* temp = topPtr;
        out = temp->data;
        topPtr = topPtr->next;
        delete temp;
        count--;
        return true;
    }

    bool peek(T& out) const {
        if (topPtr == nullptr) return false;
        out = topPtr->data;
        return true;
    }

    bool isEmpty() const { return topPtr == nullptr; }
    size_t size() const { return count; }

    void clear() {
        T tmp;
        while (pop(tmp)) {}
    }
};

/* ========================= 3. SINGLY LINKED LIST ===========================
 * USED FOR: Per-customer delivery history (append at tail, traverse in
 * chronological order, supports reverse traversal for "most recent first").
 * ========================================================================= */
template <typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& d) : data(d), next(nullptr) {}
    };
    Node* head;
    Node* tail;
    size_t count;

public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}
    ~LinkedList() { clear(); }

    void append(const T& value) {
        Node* node = new Node(value);
        if (!head) { head = tail = node; }
        else { tail->next = node; tail = node; }
        count++;
    }

    size_t size() const { return count; }
    bool isEmpty() const { return head == nullptr; }

    std::vector<T> toVectorChronological() const {
        std::vector<T> out;
        Node* cur = head;
        while (cur) { out.push_back(cur->data); cur = cur->next; }
        return out;
    }

    std::vector<T> toVectorMostRecentFirst() const {
        std::vector<T> v = toVectorChronological();
        std::vector<T> rev(v.rbegin(), v.rend());
        return rev;
    }

    void clear() {
        Node* cur = head;
        while (cur) { Node* nxt = cur->next; delete cur; cur = nxt; }
        head = tail = nullptr;
        count = 0;
    }
};

/* ======================= 4. BINARY SEARCH TREE (BST) =======================
 * USED FOR: Storing Drone records keyed by integer Drone ID, giving O(log n)
 * average insert/search/delete and an in-order traversal that yields drones
 * sorted by ID (useful for admin listing / reports).
 * ========================================================================= */
template <typename KeyT, typename ValueT>
class BinarySearchTree {
private:
    struct Node {
        KeyT key;
        ValueT value;
        Node* left;
        Node* right;
        Node(const KeyT& k, const ValueT& v) : key(k), value(v), left(nullptr), right(nullptr) {}
    };
    Node* root;
    size_t count;

    Node* insertRec(Node* node, const KeyT& key, const ValueT& value, bool& inserted) {
        if (!node) { inserted = true; return new Node(key, value); }
        if (key < node->key) node->left = insertRec(node->left, key, value, inserted);
        else if (key > node->key) node->right = insertRec(node->right, key, value, inserted);
        else { node->value = value; inserted = false; } // update existing
        return node;
    }

    Node* findRec(Node* node, const KeyT& key) const {
        if (!node) return nullptr;
        if (key == node->key) return node;
        if (key < node->key) return findRec(node->left, key);
        return findRec(node->right, key);
    }

    Node* minValueNode(Node* node) const {
        Node* cur = node;
        while (cur && cur->left) cur = cur->left;
        return cur;
    }

    Node* removeRec(Node* node, const KeyT& key, bool& removed) {
        if (!node) return nullptr;
        if (key < node->key) node->left = removeRec(node->left, key, removed);
        else if (key > node->key) node->right = removeRec(node->right, key, removed);
        else {
            removed = true;
            if (!node->left) { Node* r = node->right; delete node; return r; }
            if (!node->right) { Node* l = node->left; delete node; return l; }
            Node* successor = minValueNode(node->right);
            node->key = successor->key;
            node->value = successor->value;
            bool dummy = false;
            node->right = removeRec(node->right, successor->key, dummy);
        }
        return node;
    }

    void inorderRec(Node* node, std::vector<std::pair<KeyT, ValueT>>& out) const {
        if (!node) return;
        inorderRec(node->left, out);
        out.push_back({node->key, node->value});
        inorderRec(node->right, out);
    }

    void clearRec(Node* node) {
        if (!node) return;
        clearRec(node->left);
        clearRec(node->right);
        delete node;
    }

public:
    BinarySearchTree() : root(nullptr), count(0) {}
    ~BinarySearchTree() { clearRec(root); }

    void insert(const KeyT& key, const ValueT& value) {
        bool inserted = false;
        root = insertRec(root, key, value, inserted);
        if (inserted) count++;
    }

    bool find(const KeyT& key, ValueT& out) const {
        Node* n = findRec(root, key);
        if (!n) return false;
        out = n->value;
        return true;
    }

    ValueT* findPtr(const KeyT& key) const {
        Node* n = findRec(root, key);
        return n ? &(n->value) : nullptr;
    }

    bool remove(const KeyT& key) {
        bool removed = false;
        root = removeRec(root, key, removed);
        if (removed) count--;
        return removed;
    }

    std::vector<std::pair<KeyT, ValueT>> inorder() const {
        std::vector<std::pair<KeyT, ValueT>> out;
        inorderRec(root, out);
        return out;
    }

    size_t size() const { return count; }
    bool isEmpty() const { return root == nullptr; }
};

/* ============================== 5. HASH MAP ================================
 * Custom hash map with separate chaining (open hashing) implemented from
 * scratch on top of a std::vector<std::vector<pair>> bucket array.
 * USED FOR: O(1) average customer lookup by ID / email, and drone quick
 * lookup by ID (paired with the BST which keeps sorted order).
 * ========================================================================= */
template <typename KeyT, typename ValueT>
class HashMap {
private:
    struct Entry { KeyT key; ValueT value; bool occupied=false; };
    std::vector<std::vector<Entry>> buckets;
    size_t bucketCount;
    size_t count;

    size_t hashOf(const KeyT& key) const {
        std::hash<KeyT> hasher;
        return hasher(key) % bucketCount;
    }

    void rehashIfNeeded() {
        if (count < bucketCount * 2) return; // load factor guard
        size_t newBucketCount = bucketCount * 2;
        std::vector<std::vector<Entry>> newBuckets(newBucketCount);
        for (auto& bucket : buckets) {
            for (auto& e : bucket) {
                if (!e.occupied) continue;
                std::hash<KeyT> hasher;
                size_t idx = hasher(e.key) % newBucketCount;
                newBuckets[idx].push_back(e);
            }
        }
        buckets.swap(newBuckets);
        bucketCount = newBucketCount;
    }

public:
    explicit HashMap(size_t initialBuckets = 16) : bucketCount(initialBuckets), count(0) {
        buckets.resize(bucketCount);
    }

    void put(const KeyT& key, const ValueT& value) {
        size_t idx = hashOf(key);
        for (auto& e : buckets[idx]) {
            if (e.occupied && e.key == key) { e.value = value; return; }
        }
        Entry e; e.key = key; e.value = value; e.occupied = true;
        buckets[idx].push_back(e);
        count++;
        rehashIfNeeded();
    }

    bool get(const KeyT& key, ValueT& out) const {
        size_t idx = hashOf(key);
        for (const auto& e : buckets[idx]) {
            if (e.occupied && e.key == key) { out = e.value; return true; }
        }
        return false;
    }

    bool contains(const KeyT& key) const {
        ValueT tmp;
        return get(key, tmp);
    }

    bool remove(const KeyT& key) {
        size_t idx = hashOf(key);
        auto& bucket = buckets[idx];
        for (size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].occupied && bucket[i].key == key) {
                bucket.erase(bucket.begin() + i);
                count--;
                return true;
            }
        }
        return false;
    }

    size_t size() const { return count; }

    std::vector<std::pair<KeyT, ValueT>> items() const {
        std::vector<std::pair<KeyT, ValueT>> out;
        for (const auto& bucket : buckets)
            for (const auto& e : bucket)
                if (e.occupied) out.push_back({e.key, e.value});
        return out;
    }
};

/* ============================ 6. PRIORITY QUEUE ============================
 * Binary min-heap implemented manually over a std::vector array
 * (array-based complete binary tree, standard sift-up/sift-down).
 * USED FOR: Urgent delivery scheduling -- deliveries with a higher priority
 * value (e.g. medical / same-day) are dequeued before normal ones.
 * A custom comparator functor decides ordering (max-heap-by-priority by
 * default: highest priority number served first).
 * ========================================================================= */
template <typename T, typename Compare = std::less<T>>
class PriorityQueue {
private:
    std::vector<T> heap;
    Compare comp; // comp(a,b) == true means a should be LOWER priority than b

    void siftUp(size_t i) {
        while (i > 0) {
            size_t parent = (i - 1) / 2;
            if (comp(heap[parent], heap[i])) { std::swap(heap[parent], heap[i]); i = parent; }
            else break;
        }
    }

    void siftDown(size_t i) {
        size_t n = heap.size();
        while (true) {
            size_t left = 2 * i + 1, right = 2 * i + 2, best = i;
            if (left < n && comp(heap[best], heap[left])) best = left;
            if (right < n && comp(heap[best], heap[right])) best = right;
            if (best == i) break;
            std::swap(heap[i], heap[best]);
            i = best;
        }
    }

public:
    void push(const T& value) {
        heap.push_back(value);
        siftUp(heap.size() - 1);
    }

    bool pop(T& out) {
        if (heap.empty()) return false;
        out = heap.front();
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) siftDown(0);
        return true;
    }

    bool peek(T& out) const {
        if (heap.empty()) return false;
        out = heap.front();
        return true;
    }

    bool isEmpty() const { return heap.empty(); }
    size_t size() const { return heap.size(); }

    std::vector<T> toVector() const { return heap; }
};

#endif // DATASTRUCTURES_H
