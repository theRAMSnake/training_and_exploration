#include <iostream>
#include <string>
#include <random>
#include <unordered_set>
#include <chrono>
#include "graph_adjacency_list.hpp"
#include "graph_edge_list.hpp"
#include "garph_algo.hpp"
#include "peak_mem.hpp"
#include "graph_compressed_sparse_row.hpp"

using namespace std::chrono_literals;

constexpr int NUM_NODES = 4'000'000;
constexpr int EDGES_PER_NODE = 25;

template <typename GraphT>
void add_random_edges(GraphT& graph, int num_nodes, int edges_per_node) {
    std::mt19937_64 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> dist(0, num_nodes - 1);
    
    // First, ensure connectivity by connecting each node to the next one
    for (int u = 0; u < num_nodes - 1; ++u) {
        graph.add_edge(u, u + 1);
    }
    
    // Then add additional random edges to reach the desired edge count
    for (int u = 0; u < num_nodes; ++u) {
        std::unordered_set<int> targets;
        
        // Add random edges until we reach the desired count
        while ((int)targets.size() < edges_per_node) {
            int v = dist(rng);
            if (v != u) targets.insert(v);
        }
        
        for (int v : targets) {
            graph.add_edge(u, v);
        }
    }
}

template <typename GraphT>
void run_bfs_benchmark(const std::string& graph_name, GraphT& graph) {
    RssPoller rss{20ms};
    std::cout << "Running BFS on " << graph_name << std::endl;
    int visited = 0;
    auto start = std::chrono::steady_clock::now();
    bfs(graph, 0, [&](int) { ++visited; });
    auto end = std::chrono::steady_clock::now();
    std::cout << graph_name << ": Visited nodes: " << visited << std::endl;
    std::chrono::duration<double> elapsed = end - start;
    std::cout << graph_name << ": BFS wall time: " << elapsed.count() << " seconds" << std::endl;
    print_summary("Benchmark results");
}

void run_csr_bfs_benchmark(const std::string& graph_name, GraphCSR<int>& graph) {
    RssPoller rss{20ms};
    std::cout << "Running BFS on " << graph_name << std::endl;
    int visited = 0;
    auto start = std::chrono::steady_clock::now();
    graph.bfs(0, [&](int) { ++visited; });
    auto end = std::chrono::steady_clock::now();
    std::cout << graph_name << ": Visited nodes: " << visited << std::endl;
    std::chrono::duration<double> elapsed = end - start;
    std::cout << graph_name << ": BFS wall time: " << elapsed.count() << " seconds" << std::endl;
    print_summary("Benchmark results");
}

void run_csr_bfs_bitmask_benchmark(const std::string& graph_name, GraphCSR<int>& graph) {
    RssPoller rss{20ms};
    std::cout << "Running BFS Bitmask on " << graph_name << std::endl;
    int visited = 0;
    auto start = std::chrono::steady_clock::now();
    graph.bfs_bitmask(0, [&](int) { ++visited; });
    auto end = std::chrono::steady_clock::now();
    std::cout << graph_name << ": Visited nodes: " << visited << std::endl;
    std::chrono::duration<double> elapsed = end - start;
    std::cout << graph_name << ": BFS Bitmask wall time: " << elapsed.count() << " seconds" << std::endl;
    print_summary("Benchmark results");
}

void run_csr_bfs_threads_benchmark(const std::string& graph_name, GraphCSR<int>& graph) {
    RssPoller rss{20ms};
    std::cout << "Running BFS Threads on " << graph_name << std::endl;
    int visited = 0;
    auto start = std::chrono::steady_clock::now();
    graph.bfs_threads(0, [&](int) { ++visited; });
    auto end = std::chrono::steady_clock::now();
    std::cout << graph_name << ": Visited nodes: " << visited << std::endl;
    std::chrono::duration<double> elapsed = end - start;
    std::cout << graph_name << ": BFS Threads wall time: " << elapsed.count() << " seconds" << std::endl;
    print_summary("Benchmark results");
}

int main() {
    // Always create the adjacency list first
    std::cout << "Building base adjacency list..." << std::endl;
    GraphAdjacencyList<int> adj;
    for (int i = 0; i < NUM_NODES; ++i) adj.add_vertex(i);
    add_random_edges(adj, NUM_NODES, EDGES_PER_NODE);

    // Construct (or load) CSR
    const std::string csr_filename = "graph_csr.bin";
    GraphCSR<int> csr;
    
    // Try to load existing CSR file
    if (csr.load_from_file(csr_filename)) {
        std::cout << "Loaded CSR from file: " << csr_filename << std::endl;
    } else {
        std::cout << "Building CSR..." << std::endl;
        csr = GraphCSR<int>(adj);
        std::cout << "Saving CSR to file: " << csr_filename << std::endl;
        if (!csr.save_to_file(csr_filename)) {
            std::cerr << "Warning: Failed to save CSR to file" << std::endl;
        }
    }

    // Run benchmark functions in the specified order
    std::cout << "\n=== Starting Benchmark Suite ===" << std::endl;
    
    // 1. adj_list bfs
    //run_bfs_benchmark("adjacency_list", adj);
    
    // 2. CSR bfs
    //run_csr_bfs_benchmark("CSR", csr);
    
    // 3. CSR bfs_bitmask
    //run_csr_bfs_bitmask_benchmark("CSR", csr);
    
    // 4. CSR bfs_threads
    run_csr_bfs_threads_benchmark("CSR", csr);
    
    std::cout << "\n=== Benchmark Suite Complete ===" << std::endl;
    return 0;
} 