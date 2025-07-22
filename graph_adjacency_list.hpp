#pragma once

#include <unordered_map>
#include <set>
#include <algorithm>

template <typename T>
class GraphAdjacencyList {
public:
    /*
    * Check if two vertices are adjacent.
    * Complexity: O(n)
    */
    bool adjacent(T x, T y) const {
        auto pos = adj_list_.find(x);
        if (pos == adj_list_.end()) {
            return false;
        }
        return std::find(pos->second.begin(), pos->second.end(), y) != pos->second.end();
    }

    /*
    * Get the neighbors of a vertex.
    * Complexity: O(1)
    */
    std::vector<T> neighbors(T x) const {
        auto pos = adj_list_.find(x);
        if (pos == adj_list_.end()) {
            return {};
        }
        return pos->second;
    }

    /*
    * Add a vertex to the graph.
    * Complexity: O(1)
    */
    void add_vertex(T x) {
        adj_list_[x] = std::vector<T>();
    }

    /*
    * Remove a vertex from the graph.
    * Complexity: O(n^2)
    */
    void remove_vertex(T x) {
        for (auto it = adj_list_.begin(); it != adj_list_.end(); ++it) {
            std::erase(it->second, x);
        }
        adj_list_.erase(x);
    }

    /*
    * Add an edge to the graph.
    * Complexity: O(1)
    */
    void add_edge(T x, T y) {
        adj_list_[x].push_back(y);
        if (adj_list_.find(y) == adj_list_.end()) {
            add_vertex(y);
        }
    }

    /*
    * Remove an edge from the graph.
    * Complexity: O(n)
    */
    void remove_edge(T x, T y){
        std::erase(adj_list_[x], y);
    }

    /*
    * Get the number of vertices in the graph.
    * Complexity: O(1)
    */
    std::size_t size() const {
        return adj_list_.size();
    }

    /*
    * Get all vertices in the graph.
    * Complexity: O(n)
    */
    std::set<T> vertices() const {
        std::set<T> vertices;
        for (const auto& [vertex, _] : adj_list_) {
            vertices.insert(vertex);
        }
        return vertices;
    }

private:
    std::unordered_map<T, std::vector<T>> adj_list_;
};