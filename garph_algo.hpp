#pragma once
#include <stack>
#include <set>
#include <queue>

/*
* Depth-first search algorithm.
* Graph expected to be constant during the algorithm.
*/
template <typename G, typename T, typename O>
void dfs(const G& g, T start, O observer) {
    std::set<T> visited;
    visited.insert(start);

    std::stack<T> to_visit;
    to_visit.push(start);

    while (!to_visit.empty()) {
        const auto current = to_visit.top();
        to_visit.pop();

        observer(current);

        const auto& neighbors = g.neighbors(current);
        for (const auto& neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end()) {
                to_visit.push(neighbor);
                visited.insert(neighbor);
            }
        }
    }
}

/*
* Breadth-first search algorithm.
* Graph expected to be constant during the algorithm.
*/
template <typename G, typename T, typename O>
void bfs(const G& g, T start, O observer) {
    std::set<T> visited;
    visited.insert(start);

    std::queue<T> to_visit;
    to_visit.push(start);

    while (!to_visit.empty()) {
        const auto current = to_visit.front();
        to_visit.pop();

        observer(current);

        const auto& neighbors = g.neighbors(current);
        for (const auto& neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end()) {
                to_visit.push(neighbor);
                visited.insert(neighbor);
            }
        }
    }
}