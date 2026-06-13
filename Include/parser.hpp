#pragma once

/**
 * parser.hpp
 * ─────────────────────────────────────────────────────────────────────────────
 * Parses adjacency-list text files into Graph objects.
 *
 * Supported format (one node per line):
 *   <node_id>: <neighbor_id> <neighbor_id> ...
 *
 * Example:
 *   1: 2 3 4
 *   2: 1 19 20
 *   ...
 *
 * Rules:
 *   - Node IDs may be any non-negative integers (not necessarily 0-based).
 *   - Whitespace (spaces, tabs) between tokens is ignored.
 *   - Lines beginning with '#' are treated as comments and skipped.
 *   - Empty lines are skipped.
 *   - Edges are undirected: listing "1: 2" and "2: 1" produces one edge {1,2}.
 *   - Self-loops (node listed as its own neighbour) are silently ignored.
 */

#include "graph.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

class AdjacencyListParser {
public:
    /**
     * Parses a single adjacency-list file and returns a Graph.
     *
     * @param path  Path to the .txt file.
     * @return      Fully constructed Graph with all nodes and edges.
     * @throws      std::runtime_error on I/O or parse errors.
     */
    static Graph parse(const fs::path& path) {
        std::ifstream file{ path };
        if (!file.is_open())
            throw std::runtime_error(
                "AdjacencyListParser: cannot open '" + path.string() + "'.");

        Graph g;
        std::string line;
        int lineNum = 0;

        while (std::getline(file, line)) {
            ++lineNum;

            // Strip trailing carriage return (Windows line endings)
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#')
                continue;

            // Find the colon separating node ID from neighbour list
            const auto colonPos = line.find(':');
            if (colonPos == std::string::npos)
                throw std::runtime_error(
                    "AdjacencyListParser: missing ':' on line " +
                    std::to_string(lineNum) + " of '" + path.string() + "'.");

            // Parse source node ID
            const std::string idStr = trim(line.substr(0, colonPos));
            if (idStr.empty())
                throw std::runtime_error(
                    "AdjacencyListParser: empty node ID on line " +
                    std::to_string(lineNum) + ".");

            const Node::ID sourceId = parseId(idStr, lineNum, path);

            // Ensure the source node exists
            if (!hasNode(g, sourceId))
                g.addVertex(sourceId);

            // Parse the neighbour list (space-separated integers after ':')
            const std::string neighbourStr = line.substr(colonPos + 1);
            std::istringstream iss{ neighbourStr };
            Node::ID targetId;

            while (iss >> targetId) {
                // Ignore self-loops
                if (targetId == sourceId) continue;

                // Ensure target node exists before adding edge
                if (!hasNode(g, targetId))
                    g.addVertex(targetId);

                // addEdge deduplicates via the canonical EdgeHash set,
                // so listing both "1: 2" and "2: 1" produces one edge.
                g.addEdge(sourceId, targetId);
            }
        }

        if (g.vertexCount() == 0)
            throw std::runtime_error(
                "AdjacencyListParser: no nodes found in '" +
                path.string() + "'.");

        return g;
    }

    /**
     * Parses every .txt file in `inputDir` and returns a vector of
     * (filename_stem, Graph) pairs, sorted alphabetically by filename.
     *
     * @param inputDir  Directory containing .txt adjacency-list files.
     */
    static std::vector<std::pair<std::string, Graph>>
    parseDirectory(const fs::path& inputDir)
    {
        if (!fs::exists(inputDir) || !fs::is_directory(inputDir))
            throw std::runtime_error(
                "AdjacencyListParser: '" + inputDir.string() +
                "' is not a directory.");

        // Collect and sort .txt files for deterministic processing order
        std::vector<fs::path> files;
        for (const auto& entry : fs::directory_iterator(inputDir))
            if (entry.is_regular_file() &&
                entry.path().extension() == ".txt")
                files.push_back(entry.path());

        std::sort(files.begin(), files.end());

        std::vector<std::pair<std::string, Graph>> result;
        result.reserve(files.size());

        for (const auto& p : files) {
            std::cout << "  Parsing: " << p.filename().string() << " ... ";
            std::cout.flush();
            Graph g = parse(p);
            std::cout << "|V|=" << g.vertexCount()
                      << "  |E|=" << g.edgeCount() << '\n';
            result.emplace_back(p.stem().string(), std::move(g));
        }

        return result;
    }

private:
    // ── Helpers ───────────────────────────────────────────────

    // Returns true if the graph already contains a vertex with this ID.
    // Graph does not expose a hasVertex() method so we probe via addVertex.
    static bool hasNode(const Graph& g, Node::ID id) {
        for (const Node& v : g.nodes())
            if (v.id == id) return true;
        return false;
    }

    static Node::ID parseId(const std::string& s, int line,
                            const fs::path& path)
    {
        try {
            const unsigned long val = std::stoul(s);
            return static_cast<Node::ID>(val);
        } catch (...) {
            throw std::runtime_error(
                "AdjacencyListParser: invalid node ID '" + s +
                "' on line " + std::to_string(line) +
                " of '" + path.string() + "'.");
        }
    }

    static std::string trim(std::string s) {
        const auto notSpace = [](unsigned char c){ return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
        return s;
    }
};