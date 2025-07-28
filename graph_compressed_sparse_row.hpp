#pragma once
#include <vector>
#include <set>
#include <algorithm>
#include "graph_adjacency_list.hpp"
#include <queue>
#include <fstream>
#include <iostream>

/*
2M x 15 edges	
Building base adjacency list...
Loaded CSR from file: graph_csr.bin
Running BFS
csr: Visited nodes: 2000000
csr: BFS wall time: 12.5187 seconds
=== Benchmark results ===
Peak RSS        : 591.625 MiB
Peak live heap  : 478.094 MiB
*/
/* 
 * Stores matrix in a compressed format
 * Example:
 * [ 0, 0, 1, 0]
 * [ 1, 1, 0, 0]
 * [ 0, 0, 0, 0]
 * [ 1, 1, 1, 1]
 *
 * will be stored as:
 * row_ptrs_:
 * [0, 1, 3, 3, 7]
 *
 * column_indices_
 * [2, 0, 1, 0, 1, 2, 3]
 *
 * values_
 * [0, 1, 2, 3]
 */
template<typename T, typename S = std::make_unsigned<T>::type>
class GraphCSR {
public:
    static_assert(sizeof(S) <= sizeof(std::size_t), "S must be smaller or equal than std::size_t");
    GraphCSR() {
        row_ptrs_.push_back(0);
    }

    GraphCSR(const GraphAdjacencyList<T>& graph) {
        values_.reserve(graph.size());
        auto ver_src = graph.vertices();
        for(auto x : ver_src) {
            values_.push_back(x);
        }

        row_ptrs_.reserve(graph.size() + 1);
        row_ptrs_.push_back(0);
        column_indices_.reserve(graph.count_edges());

        for(auto x : ver_src) {
            auto ns = graph.neighbors(x);
            std::sort(ns.begin(), ns.end());

            for(auto y : ns) {
                column_indices_.push_back(value_index(y));
            }
            row_ptrs_.push_back(static_cast<S>(column_indices_.size()));
        }
    }

    /*
     * Returns true if edge exists from x to y. 
     * Returns false if nodes does not exist or if there are no edge from x to y.
     * Complexity: log(n)
     */
    bool adjacent(T x, T y) const {
        auto x_idx = value_index(x);
        auto y_idx = value_index(y);

        if (x_idx == size() || y_idx == size()) {
            return false;
        }

        auto [b, e] = row_begin_end(x_idx);
        return std::binary_search(b, e, y_idx);
    }

    std::vector<T> neighbors(T x) const {
        auto x_idx = value_index(x);
        if (x_idx == size()) {
            return {};
        }
        auto [b, e] = row_begin_end(x_idx);

        std::vector<T> result(e - b);
        std::size_t pos = 0;
        for (auto i = b; i != e; ++i, ++pos) {
            result[pos] = values_[*i];
        }
        return result;
    }

    void add_vertex(T x) {
        values_.push_back(x);
        row_ptrs_.push_back(row_ptrs_.back());
    }

    void remove_vertex(T x) {
        auto idx = value_index(x);
        if (idx == size()) return;

        auto [b, e] = row_begin_end(idx);
        auto num_cols = e - b;
        column_indices_.erase(b, e);

        for (auto i = idx; i != row_ptrs_.size(); ++i) {
            row_ptrs_[i] -= static_cast<S>(num_cols);
        }

        row_ptrs_.erase(row_ptrs_.begin() + idx);
        values_.erase(values_.begin() + idx);
   }

    void add_edge(T x, T y) {
        auto x_idx = value_index(x);
        auto y_idx = value_index(y);

        if (x_idx == size() || y_idx == size()) {
            return;
        }

        auto [b, e] = row_begin_end(x_idx);
        column_indices_.insert(std::lower_bound(b, e, y_idx), y_idx);

        for (auto i = x_idx + 1; i != row_ptrs_.size(); ++i) {
            row_ptrs_[i]++;
        }
    }

    void remove_edge(T x, T y) {
        auto x_idx = value_index(x);
        auto y_idx = value_index(y);

        if (x_idx == size() || y_idx == size()) {
            return;
        }
        auto [b, e] = row_begin_end(x_idx);
        auto pos = std::lower_bound(b, e, y_idx);
        if (pos != e && *pos == y_idx) {
            column_indices_.erase(pos);
            for (auto i = x_idx + 1; i != row_ptrs_.size(); ++i) {
                row_ptrs_[i]--;
            }
        }
    }

    /*
     * Get the number of vertices in the graph.
     * Complexity: O(1)
     */
    S size() const {
        return static_cast<S>(values_.size());
    }

    /*
     * Save the CSR graph to a binary file.
     * Returns true on success, false on failure.
     */
    bool save_to_file(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }

        // Write the size of each vector
        S values_size = static_cast<S>(values_.size());
        S row_ptrs_size = static_cast<S>(row_ptrs_.size());
        S column_indices_size = static_cast<S>(column_indices_.size());

        file.write(reinterpret_cast<const char*>(&values_size), sizeof(S));
        file.write(reinterpret_cast<const char*>(&row_ptrs_size), sizeof(S));
        file.write(reinterpret_cast<const char*>(&column_indices_size), sizeof(S));

        // Write the data vectors
        file.write(reinterpret_cast<const char*>(values_.data()), values_size * sizeof(T));
        file.write(reinterpret_cast<const char*>(row_ptrs_.data()), row_ptrs_size * sizeof(S));
        file.write(reinterpret_cast<const char*>(column_indices_.data()), column_indices_size * sizeof(S));

        file.close();
        return true;
    }

    /*
     * Load the CSR graph from a binary file.
     * Returns true on success, false on failure.
     */
    bool load_from_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filename << std::endl;
            return false;
        }

        // Read the size of each vector
        S values_size, row_ptrs_size, column_indices_size;
        file.read(reinterpret_cast<char*>(&values_size), sizeof(S));
        file.read(reinterpret_cast<char*>(&row_ptrs_size), sizeof(S));
        file.read(reinterpret_cast<char*>(&column_indices_size), sizeof(S));

        // Resize vectors and read data
        values_.resize(values_size);
        row_ptrs_.resize(row_ptrs_size);
        column_indices_.resize(column_indices_size);

        file.read(reinterpret_cast<char*>(values_.data()), values_size * sizeof(T));
        file.read(reinterpret_cast<char*>(row_ptrs_.data()), row_ptrs_size * sizeof(S));
        file.read(reinterpret_cast<char*>(column_indices_.data()), column_indices_size * sizeof(S));

        file.close();
        return true;
    }

    std::set<T> vertices() const {
        std::set<T> vertices;
        for (S i = 0; i < values_.size(); ++i) {
            vertices.insert(values_[i]);
        }
        return vertices;
    }

    template<typename O>
    void bfs(T start, O observer) {
        auto pos = value_index(start);

        std::set<S> visited;
        visited.insert(pos);

        std::queue<S> to_visit;
        to_visit.push(pos);

        while (!to_visit.empty()) {
            const auto current_pos = to_visit.front();
            to_visit.pop();

            observer(values_[current_pos]);

            auto [b, e] = row_begin_end(current_pos);
            for (auto i = b; i != e; ++i) {
                if (visited.find(*i) == visited.end()) {
                    to_visit.push(*i);
                    visited.insert(*i);
                }
            }
        }
    }

private:
    std::pair<
        typename std::vector<S>::const_iterator,
        typename std::vector<S>::const_iterator
    > row_begin_end(S x) const {
        auto start = row_ptrs_[x];
        auto end = row_ptrs_[x + 1];
        return {column_indices_.begin() + start, column_indices_.begin() + end};
    }

    S value_index(T x) const {
        for (S i = 0; i < values_.size(); ++i) {
            if (values_[i] == x) {
                return i;
            }
        }
        return size();
    }

    // For every row X, contains first index of its column-match in the column_indices_ array;
    std::vector<S> row_ptrs_;

    // Contains every column-match of every row. Use row_ptrs_ array to find respective row;
    std::vector<S> column_indices_;

    // Contains values field for every vertex
    std::vector<T> values_;
};
