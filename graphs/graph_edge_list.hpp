#pragma once

#include <unordered_map>
#include <set>
#include <limits>

template <typename T>
class GraphEdgeList {
public:
    /*
    * Check if two vertices are adjacent.
    * Complexity: O(n)
    */
    bool adjacent(T x, T y) const {
        return std::find(edges_.begin(), edges_.end(), std::make_pair(x, y)) != edges_.end();
    }

    /*
    * Get the neighbors of a vertex.
    * Complexity: O(n)
    */
    std::vector<T> neighbors(T x) const {
        std::vector<T> neighbors;
        for (const auto& edge : edges_) {
            if (edge.first == x && edge.second != std::numeric_limits<T>::max()) {
                neighbors.push_back(edge.second);
            }
        }
        return neighbors;
    }

    /*
    * Add a vertex to the graph.
    * Complexity: O(1)
    */
    void add_vertex(T x) {
        add_edge(x, std::numeric_limits<T>::max());
    }

    /*
    * Remove a vertex from the graph.
    * Complexity: O(n)
    */
    void remove_vertex(T x) {
        edges_.erase(std::remove_if(edges_.begin(), edges_.end(), [x](const auto& edge) {
            return edge.first == x || edge.second == x;
        }), edges_.end());
    }

    /*
    * Add an edge to the graph.
    * Complexity: O(1)
    */
    void add_edge(T x, T y) {
        edges_.push_back(std::make_pair(x, y));
    }

    /*
    * Remove an edge from the graph.
    * Complexity: O(n)
    */
    void remove_edge(T x, T y){
        std::erase(edges_, std::make_pair(x, y));
    }

    /*
    * Get the number of vertices in the graph.
    * Complexity: O(n)
    */
    std::size_t size() const {
        return vertices().size();
    }

    /*
    * Get all vertices in the graph.
    * Complexity: O(n)
    */
    std::set<T> vertices() const {
        std::set<T> vertices;
        for (const auto& edge : edges_) {
            vertices.insert(edge.first);
            if (edge.second != std::numeric_limits<T>::max()) {
                vertices.insert(edge.second);
            }
        }
        return vertices;
    }

private:
    std::vector<std::pair<T, T>> edges_;
};