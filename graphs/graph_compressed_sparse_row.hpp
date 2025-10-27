#pragma once
#include <vector>
#include <set>
#include <algorithm>
#include "graph_adjacency_list.hpp"
#include <queue>
#include <fstream>
#include <iostream>
#include <barrier>
#include <thread>
#include <atomic>

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
        std::unordered_map<T, S> value_to_index;
        auto iter = ver_src.begin();
        for(S i = 0; i < static_cast<S>(ver_src.size()); ++i) {
            values_.push_back(*iter);
            value_to_index[*iter] = i;
            ++iter;
        }

        row_ptrs_.reserve(graph.size() + 1);
        row_ptrs_.push_back(0);
        column_indices_.reserve(graph.count_edges());

        for(auto x : ver_src) {
            auto ns = graph.neighbors(x);
            std::sort(ns.begin(), ns.end());

            for(auto y : ns) {
                column_indices_.push_back(value_to_index[y]);
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

    /*
     * General purpose BFS algorithm.
     * 2M x 15 edges	
     * csr: BFS wall time: 9.98535 seconds
     * Additional memory: N*S in the worst case.
     */
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
                auto [iter, inserted] = visited.insert(*i);
                if (inserted) {
                    to_visit.push(*i);
                }
            }
        }
    }

    /*
     * BFS with bitmask.
     * Additional memory: always N/8 bytes.
     * Beats general purpose when average component size is greater than S*8.
     * For 32 bits S its 32.
     * 2M x 15 edges	
     * csr: BFS wall time: 0.233038 seconds
     * 4M x 25 edges
     * csr: BFS wall time: 0.723602 seconds
     * Improvements history:
     * 1. Removed set and replaced with bitmask. 9.98s -> 0.23s
     */
    template<typename O>
    void bfs_bitmask(T start, O observer) {
        auto pos = value_index(start);

        std::vector<bool> visited(size(), false);
        visited[pos] = true;

        std::queue<S> to_visit;
        to_visit.push(pos);

        while (!to_visit.empty()) {
            const auto current_pos = to_visit.front();
            to_visit.pop();

            observer(values_[current_pos]);

            auto [b, e] = row_begin_end(current_pos);
            for (auto i = b; i != e; ++i) {
                if (!visited[*i]) {
                    visited[*i] = true;
                    to_visit.push(*i);
                }
            }
        }
    }

    /*
     * BFS with threads. Kept for historical reasons.
     * 4M x 25 edges
     * Wall time: 0.39 - 0.42 seconds
     */
    template<typename O>
    void bfs_threads(T start, O observer) {
        const auto num_threads = 4;

        auto pos = value_index(start);
        std::vector<std::atomic<std::uint64_t>> visited(size() / 64 + 1);

        std::vector<S> to_visit;
        to_visit.push_back(pos);
        observer(values_[pos]);
        visited[pos / 64].fetch_or(std::uint64_t(1) << pos % 64);

        std::vector<S> next_visit[num_threads];

        auto sync_func = [&]() noexcept {
            to_visit.clear();
            for(auto i = 0; i < num_threads; ++i) {
                to_visit.insert(to_visit.end(), next_visit[i].begin(), next_visit[i].end());
                next_visit[i].clear();
            }

            // One might think that by sorting to visit list we will improve cache locality,
            // as we will be visiting nodes that are close to each other and the array will be
            // split in 4 almost equal non intersecting parts.
            // However, the experiment shows that we can be 40% faster without sorting here.
            //std::sort(to_visit.begin(), to_visit.end());

            for(auto x : to_visit) {
                observer(values_[x]);
            }
        };

        std::barrier barrier(num_threads, sync_func);

        const auto thread_func = [&](auto idx) {
           while(!to_visit.empty()) {
             const auto thread_share = to_visit.size() / num_threads;
             const auto my_start = idx * thread_share;
             const auto my_end = idx == num_threads - 1 ? to_visit.size() : my_start + thread_share;

             for(auto i = my_start; i != my_end; ++i) {
                auto v = to_visit[i];
                auto [b, e] = row_begin_end(v);
                for(auto j = b; j != e; ++j) {
                    auto n = *j;
                    //TODO: Snake, find a way not to do this.
                    auto prev_val = visited[n / 64].fetch_or(std::uint64_t(1) << n % 64);
                    if((prev_val & (std::uint64_t(1) << n % 64)) == 0) {
                        next_visit[idx].push_back(n);
                    }
                }
             }

             barrier.arrive_and_wait();
           }
        };

        std::vector<std::jthread> threads;
        threads.reserve(num_threads);
        for(int i = 0; i < num_threads; ++i) {
            threads.emplace_back(thread_func, i);
        }
    }

    /*
     * BFS with threads v2.
     * In this version each thread does not set visited bit (to avoid highly contested sharing), but instead drops
     * next to visit neighbor to its thread local list. The sync thread merges all the lists and sets the visited bit.
     * 4M x 25 edges
     * Wall time: 0.31 - 0.32 seconds
     * This bottlenecks on:
     * auto v = to_visit[i];
     * in the thread function
     * with 15.933.010      cpu_core/LLC-load-misses/        #   68,89% of all LL-cache accesses
     */
    template<typename O>
    void bfs_threads_v2(T start, O observer) {
        const auto num_threads = 4;

        auto pos = value_index(start);
        std::vector<std::uint64_t> visited(size() / 64 + 1);

        std::vector<S> to_visit;
        to_visit.push_back(pos);
        observer(values_[pos]);
        visited[pos / 64] |= std::uint64_t(1) << pos % 64;

        std::vector<S> next_visit[num_threads];

        auto sync_func = [&]() noexcept {
            to_visit.clear();
            for(auto i = 0; i < num_threads; ++i) {
                for(auto x : next_visit[i]) {
                    if((visited[x / 64] & (std::uint64_t(1) << x % 64)) == 0) {
                        to_visit.push_back(x);
                        visited[x / 64] |= std::uint64_t(1) << x % 64;
                    }
                }
                next_visit[i].clear();
            }

            // One might think that by sorting to visit list we will improve cache locality,
            // as we will be visiting nodes that are close to each other and the array will be
            // split in 4 almost equal non intersecting parts.
            // However, the experiment shows that we can be 40% faster without sorting here.
            //std::sort(to_visit.begin(), to_visit.end());

            for(auto x : to_visit) {
                observer(values_[x]);
            }
        };

        std::barrier barrier(num_threads, sync_func);

        const auto thread_func = [&](auto idx) {
           while(!to_visit.empty()) {
             const auto thread_share = to_visit.size() / num_threads;
             const auto my_start = idx * thread_share;
             const auto my_end = idx == num_threads - 1 ? to_visit.size() : my_start + thread_share;

             for(auto i = my_start; i != my_end; ++i) {
                auto v = to_visit[i];
                auto [b, e] = row_begin_end(v);
                for(auto j = b; j != e; ++j) {
                    auto n = *j;
                    if((visited[n / 64] & (std::uint64_t(1) << n % 64)) == 0) {
                        next_visit[idx].push_back(n);
                    }
                }
             }

             barrier.arrive_and_wait();
           }
        };

        std::vector<std::jthread> threads;
        threads.reserve(num_threads);
        for(int i = 0; i < num_threads; ++i) {
            threads.emplace_back(thread_func, i);
        }
    }

    /*
     * BFS with threads v3.
     * In this version the share of the visiting list for the iteration is changed from a fixed percentage, i.e.
     * first thread iterates first 25%, second - 25-50% and so on,
     * to the 
     * first thread iterated modulo of its index elements.
     * This improves cache locality and reduces LLC load misses as we are keeping only small parts of the visiting list in the cache.
     * 4M x 25 edges
     * Wall time: 0.23 - 0.25 seconds
     * Bottleneck (1):
     * if((visited[n / 64] & (std::uint64_t(1) << n % 64)) == 0)
     * It accounts for 40% exec time and LLC load misses.
     * Other potential bottleneck (2): 
     * I see the hotspot for hitm loads in "next_visit[idx].push_back(n);" that is possibly explained with the size vs capacity check in
     * each vector push_back, as next_visit vectors are in array and might be part of the same cache line. Fixing this degrades performance.
     */
    template<typename O>
    void bfs_threads_v3(T start, O observer) {
        const auto num_threads = 4;

        auto pos = value_index(start);
        std::vector<std::uint64_t> visited(size() / 64 + 1);

        std::vector<S> to_visit;
        to_visit.push_back(pos);
        observer(values_[pos]);
        visited[pos / 64] |= std::uint64_t(1) << pos % 64;

        std::vector<S> next_visit[num_threads];

        auto sync_func = [&]() noexcept {
            to_visit.clear();
            for(auto i = 0; i < num_threads; ++i) {
                for(auto x : next_visit[i]) {
                    if((visited[x / 64] & (std::uint64_t(1) << x % 64)) == 0) {
                        to_visit.push_back(x);
                        observer(values_[x]);
                        visited[x / 64] |= std::uint64_t(1) << x % 64;
                    }
                }
                next_visit[i].clear();
            }
        };

        std::barrier barrier(num_threads, sync_func);

        const auto thread_func = [&](auto idx) {
           while(!to_visit.empty()) {
             for(auto i = idx; i < to_visit.size(); i += num_threads) {
                auto v = to_visit[i];
                auto [b, e] = row_begin_end(v);
                for(auto j = b; j != e; ++j) {
                    auto n = *j;
                    if((visited[n / 64] & (std::uint64_t(1) << n % 64)) == 0) {
                        next_visit[idx].push_back(n);
                    }
                }
             }

             barrier.arrive_and_wait();
           }
        };

        std::vector<std::jthread> threads;
        threads.reserve(num_threads);
        for(int i = 0; i < num_threads; ++i) {
            threads.emplace_back(thread_func, i);
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
