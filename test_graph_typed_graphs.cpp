#include "graph_adjacency_list.hpp"
#include "graph_edge_list.hpp"
#include "graph_compressed_sparse_row.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <set>
#include "garph_algo.hpp"

// List of graph types to test
using GraphTypes = ::testing::Types<GraphAdjacencyList<int>, GraphEdgeList<int>, GraphCSR<int>>;

template <typename T>
class GraphTest : public ::testing::Test {
public:
    T g;
};
TYPED_TEST_SUITE(GraphTest, GraphTypes);

TYPED_TEST(GraphTest, AddAndRemoveVertex) {
    EXPECT_EQ(this->g.size(), 0);
    EXPECT_TRUE(this->g.vertices().empty());
    this->g.add_vertex(1);
    EXPECT_EQ(this->g.size(), 1);
    EXPECT_EQ(this->g.vertices(), std::set<int>({1}));
    this->g.add_vertex(2);
    EXPECT_EQ(this->g.size(), 2);
    EXPECT_EQ(this->g.vertices(), (std::set<int>{1, 2}));
    EXPECT_TRUE(this->g.neighbors(1).empty());
    EXPECT_TRUE(this->g.neighbors(2).empty());
    this->g.remove_vertex(1);
    // For edge list, size() may not decrease unless edges existed, but vertices() should not contain 1
    auto verts = this->g.vertices();
    EXPECT_TRUE(verts.find(1) == verts.end());
    EXPECT_TRUE(this->g.neighbors(1).empty());
}

TYPED_TEST(GraphTest, AddDuplicateVertex) {
    this->g.add_vertex(1);
    this->g.add_vertex(1); // Should not add duplicate
    EXPECT_EQ(this->g.vertices(), std::set<int>({1}));
}

TYPED_TEST(GraphTest, AddAndRemoveEdge) {
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_edge(1, 2);
    auto neighbors = this->g.neighbors(1);
    EXPECT_TRUE(std::find(neighbors.begin(), neighbors.end(), 2) != neighbors.end());
    EXPECT_TRUE(this->g.adjacent(1, 2));
    EXPECT_EQ(this->g.vertices(), (std::set<int>{1, 2}));
    this->g.remove_edge(1, 2);
    EXPECT_FALSE(this->g.adjacent(1, 2));
    neighbors = this->g.neighbors(1);
    EXPECT_TRUE(std::find(neighbors.begin(), neighbors.end(), 2) == neighbors.end());
    EXPECT_EQ(this->g.vertices(), (std::set<int>{1, 2}));
}

TYPED_TEST(GraphTest, AddDuplicateEdge) {
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_edge(1, 2);
    this->g.add_edge(1, 2); // Allow duplicate edges (multigraph behavior)
    auto neighbors = this->g.neighbors(1);
    EXPECT_GE(std::count(neighbors.begin(), neighbors.end(), 2), 1);
    EXPECT_EQ(this->g.vertices(), (std::set<int>{1, 2}));
}

TYPED_TEST(GraphTest, RemoveNonexistentEdge) {
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.remove_edge(1, 2); // Should not throw
    EXPECT_FALSE(this->g.adjacent(1, 2));
    EXPECT_EQ(this->g.vertices(), (std::set<int>{1, 2}));
}

TYPED_TEST(GraphTest, RemoveNonexistentVertex) {
    this->g.add_vertex(1);
    this->g.remove_vertex(2); // Should not throw
    EXPECT_EQ(this->g.vertices(), std::set<int>({1}));
}

TYPED_TEST(GraphTest, SelfLoop) {
    this->g.add_vertex(1);
    this->g.add_edge(1, 1);
    EXPECT_TRUE(this->g.adjacent(1, 1));
    auto neighbors = this->g.neighbors(1);
    EXPECT_GE(std::count(neighbors.begin(), neighbors.end(), 1), 1);
    EXPECT_EQ(this->g.vertices(), std::set<int>({1}));
    this->g.remove_edge(1, 1);
    EXPECT_FALSE(this->g.adjacent(1, 1));
    EXPECT_EQ(this->g.vertices(), std::set<int>({1}));
}

TYPED_TEST(GraphTest, EmptyGraph) {
    EXPECT_EQ(this->g.size(), 0);
    EXPECT_TRUE(this->g.vertices().empty());
    EXPECT_TRUE(this->g.neighbors(42).empty());
    EXPECT_FALSE(this->g.adjacent(1, 2));
}

TYPED_TEST(GraphTest, ComplexGraphStructure) {
    for (int i = 1; i <= 5; ++i) this->g.add_vertex(i);
    this->g.add_edge(1, 2);
    this->g.add_edge(1, 3);
    this->g.add_edge(2, 4);
    this->g.add_edge(3, 4);
    this->g.add_edge(4, 5);
    EXPECT_TRUE(this->g.adjacent(1, 2));
    EXPECT_TRUE(this->g.adjacent(1, 3));
    EXPECT_TRUE(this->g.adjacent(2, 4));
    EXPECT_TRUE(this->g.adjacent(3, 4));
    EXPECT_TRUE(this->g.adjacent(4, 5));
    EXPECT_FALSE(this->g.adjacent(5, 1));
    auto n1 = this->g.neighbors(1);
    EXPECT_TRUE(std::find(n1.begin(), n1.end(), 2) != n1.end());
    EXPECT_TRUE(std::find(n1.begin(), n1.end(), 3) != n1.end());
    auto n4 = this->g.neighbors(4);
    EXPECT_TRUE(std::find(n4.begin(), n4.end(), 5) != n4.end());
    EXPECT_EQ(this->g.vertices(), (std::set<int>{1, 2, 3, 4, 5}));
}

TYPED_TEST(GraphTest, VerticesEmptyGraph) {
    auto verts = this->g.vertices();
    EXPECT_TRUE(verts.empty());
}

TYPED_TEST(GraphTest, VerticesAfterAdd) {
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    std::set<int> expected{1, 2, 3};
    auto verts = this->g.vertices();
    EXPECT_EQ(verts, expected);
}

TYPED_TEST(GraphTest, VerticesAfterRemoveVertex) {
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.remove_vertex(2);
    std::set<int> expected{1, 3};
    auto verts = this->g.vertices();
    EXPECT_EQ(verts, expected);
}

TYPED_TEST(GraphTest, DFSAlgorithm) {
    // Build a graph: 1 -> 2, 1 -> 3, 2 -> 4, 3 -> 4, 4 -> 5
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.add_vertex(4);
    this->g.add_vertex(5);
    this->g.add_edge(1, 2);
    this->g.add_edge(1, 3);
    this->g.add_edge(2, 4);
    this->g.add_edge(3, 4);
    this->g.add_edge(4, 5);

    std::vector<int> visit_order;
    auto observer = [&](int v) { visit_order.push_back(v); };
    dfs(this->g, 1, observer);

    // All nodes should be visited
    std::set<int> visited(visit_order.begin(), visit_order.end());
    EXPECT_EQ(visited, (std::set<int>{1, 2, 3, 4, 5}));
    EXPECT_EQ(visit_order.front(), 1);
    EXPECT_TRUE(visit_order.back() == 2 || visit_order.back() == 3);
}

TYPED_TEST(GraphTest, DFSAlgorithm_OrderAndCoverage) {
    // Graph: 1 -> 2, 1 -> 3, 2 -> 4, 3 -> 4, 4 -> 5
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.add_vertex(4);
    this->g.add_vertex(5);
    this->g.add_edge(1, 2);
    this->g.add_edge(1, 3);
    this->g.add_edge(2, 4);
    this->g.add_edge(3, 4);
    this->g.add_edge(4, 5);

    // DFS from 1: order depends on stack (LIFO), so 3 or 2 can be visited first after 1
    // For this implementation, neighbors are pushed in the order returned by neighbors(), so 2 then 3
    // So stack: push 2, push 3 -> pop 3, pop 2
    // So expected order: 1, 3, 4, 5, 2 or 1, 2, 4, 5, 3 depending on neighbors order
    // Let's check both possibilities
    std::vector<int> visit_order;
    dfs(this->g, 1, [&](int v) { visit_order.push_back(v); });
    std::vector<std::vector<int>> possible_orders = {
        {1, 3, 4, 5, 2},
        {1, 2, 4, 5, 3}
    };
    bool matches = std::find(possible_orders.begin(), possible_orders.end(), visit_order) != possible_orders.end();
    EXPECT_TRUE(matches) << "DFS order was: " << ::testing::PrintToString(visit_order);
    EXPECT_EQ(std::set<int>(visit_order.begin(), visit_order.end()), (std::set<int>{1,2,3,4,5}));

    // DFS from 4 (should only visit 4,5)
    visit_order.clear();
    dfs(this->g, 4, [&](int v) { visit_order.push_back(v); });
    EXPECT_EQ(visit_order, (std::vector<int>{4,5}));

    // DFS from 5 (should only visit 5)
    visit_order.clear();
    dfs(this->g, 5, [&](int v) { visit_order.push_back(v); });
    EXPECT_EQ(visit_order, (std::vector<int>{5}));
}

TYPED_TEST(GraphTest, DFSAlgorithm_Cycle) {
    // Graph: 1 -> 2 -> 3 -> 1 (cycle), 3 -> 4
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.add_vertex(4);
    this->g.add_edge(1, 2);
    this->g.add_edge(2, 3);
    this->g.add_edge(3, 1);
    this->g.add_edge(3, 4);
    std::vector<int> visit_order;
    dfs(this->g, 1, [&](int v) { visit_order.push_back(v); });
    // Should visit all nodes, not get stuck in cycle
    EXPECT_EQ(std::set<int>(visit_order.begin(), visit_order.end()), (std::set<int>{1,2,3,4}));
    // Should start at 1 and end at 4
    EXPECT_EQ(visit_order.front(), 1);
    EXPECT_EQ(visit_order.back(), 4);
}

TYPED_TEST(GraphTest, DFSAlgorithm_Disconnected) {
    // Graph: 1->2, 3->4 (disconnected components)
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.add_vertex(4);
    this->g.add_edge(1, 2);
    this->g.add_edge(3, 4);
    std::vector<int> visit_order;
    dfs(this->g, 1, [&](int v) { visit_order.push_back(v); });
    // Should only visit 1 and 2
    EXPECT_EQ(std::set<int>(visit_order.begin(), visit_order.end()), (std::set<int>{1,2}));
    visit_order.clear();
    dfs(this->g, 3, [&](int v) { visit_order.push_back(v); });
    EXPECT_EQ(std::set<int>(visit_order.begin(), visit_order.end()), (std::set<int>{3,4}));
} 

TYPED_TEST(GraphTest, BFSAlgorithm) {
    // Build a graph: 1 -> 2, 1 -> 3, 2 -> 4, 3 -> 4, 4 -> 5
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.add_vertex(4);
    this->g.add_vertex(5);
    this->g.add_edge(1, 2);
    this->g.add_edge(1, 3);
    this->g.add_edge(2, 4);
    this->g.add_edge(3, 4);
    this->g.add_edge(4, 5);

    std::vector<int> visit_order;
    bfs(this->g, 1, [&](int v) { visit_order.push_back(v); });
    // BFS order should be [1, 2, 3, 4, 5]
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_EQ(visit_order, expected);
}

TYPED_TEST(GraphTest, BFSAlgorithm_OrderAndCoverage) {
    // Graph: 1 -> 2, 1 -> 3, 2 -> 4, 3 -> 4, 4 -> 5
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.add_vertex(4);
    this->g.add_vertex(5);
    this->g.add_edge(1, 2);
    this->g.add_edge(1, 3);
    this->g.add_edge(2, 4);
    this->g.add_edge(3, 4);
    this->g.add_edge(4, 5);

    std::vector<int> visit_order;
    // BFS from 1
    bfs(this->g, 1, [&](int v) { visit_order.push_back(v); });
    EXPECT_EQ(visit_order, (std::vector<int>{1, 2, 3, 4, 5}));
    // BFS from 4 (should only visit 4,5)
    visit_order.clear();
    bfs(this->g, 4, [&](int v) { visit_order.push_back(v); });
    EXPECT_EQ(visit_order, (std::vector<int>{4, 5}));
    // BFS from 5 (should only visit 5)
    visit_order.clear();
    bfs(this->g, 5, [&](int v) { visit_order.push_back(v); });
    EXPECT_EQ(visit_order, (std::vector<int>{5}));
}

TYPED_TEST(GraphTest, BFSAlgorithm_Cycle) {
    // Graph: 1 -> 2 -> 3 -> 1 (cycle), 3 -> 4
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.add_vertex(4);
    this->g.add_edge(1, 2);
    this->g.add_edge(2, 3);
    this->g.add_edge(3, 1);
    this->g.add_edge(3, 4);
    std::vector<int> visit_order;
    bfs(this->g, 1, [&](int v) { visit_order.push_back(v); });
    // Should visit all nodes, not get stuck in cycle, and in BFS order
    EXPECT_EQ(visit_order, (std::vector<int>{1, 2, 3, 4}));
}

TYPED_TEST(GraphTest, BFSAlgorithm_Disconnected) {
    // Graph: 1->2, 3->4 (disconnected components)
    this->g.add_vertex(1);
    this->g.add_vertex(2);
    this->g.add_vertex(3);
    this->g.add_vertex(4);
    this->g.add_edge(1, 2);
    this->g.add_edge(3, 4);
    std::vector<int> visit_order;
    bfs(this->g, 1, [&](int v) { visit_order.push_back(v); });
    // Should only visit 1 and 2
    EXPECT_EQ(visit_order, (std::vector<int>{1, 2}));
    visit_order.clear();
    bfs(this->g, 3, [&](int v) { visit_order.push_back(v); });
    EXPECT_EQ(visit_order, (std::vector<int>{3, 4}));
} 

// Test: Large AdjacencyList to CSR equivalence
TEST(GraphConversionTest, LargeAdjacencyListToCSREquivalence) {
    constexpr int num_nodes = 200;
    constexpr int num_edges = 500;
    GraphAdjacencyList<int> adj;
    // Add nodes
    for (int i = 0; i < num_nodes; ++i) {
        adj.add_vertex(i);
    }
    // Add edges in a pseudo-random but deterministic way
    int edge_count = 0;
    for (int i = 0; i < num_nodes && edge_count < num_edges; ++i) {
        for (int j = 1; j <= 5 && edge_count < num_edges; ++j) {
            int to = (i * j + 17) % num_nodes;
            if (to != i && !adj.adjacent(i, to)) {
                adj.add_edge(i, to);
                ++edge_count;
            }
        }
    }
    // Construct CSR from adjacency list
    GraphCSR<int> csr(adj);
    // Check vertices are identical
    EXPECT_EQ(adj.vertices(), csr.vertices());
    // Check adjacency and neighbors for all pairs
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            EXPECT_EQ(adj.adjacent(i, j), csr.adjacent(i, j)) << "Mismatch at adjacent(" << i << ", " << j << ")";
        }
        auto adj_neighbors = adj.neighbors(i);
        auto csr_neighbors = csr.neighbors(i);
        std::sort(adj_neighbors.begin(), adj_neighbors.end());
        std::sort(csr_neighbors.begin(), csr_neighbors.end());
        EXPECT_EQ(adj_neighbors, csr_neighbors) << "Mismatch in neighbors(" << i << ")";
    }
} 