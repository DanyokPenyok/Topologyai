#include "pathfinder.h"
#include <QDebug>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include "graph.h"

std::string Path::toString() const {
    std::string res = "Path: ";
    for (size_t i = 0; i < vertices.size(); ++i) {
        res += std::to_string(vertices[i]);
        if (i < vertices.size() - 1)
            res += " -> ";
    }
    return res;
}

std::string CutSet::toString() const {
    std::string res = "Cut: {";
    for (size_t i = 0; i < edges.size(); ++i) {
        res += "(" + std::to_string(edges[i].first) + "," + std::to_string(edges[i].second) + ")";
        if (i < edges.size() - 1)
            res += ", ";
    }
    return res + "}";
}

PathFinder::PathFinder(const Graph &graph) : m_graph(graph) {}

std::vector<Path> PathFinder::findAllPaths(unsigned int source, unsigned int sink) {
    std::vector<Path> allPaths;
    if (source >= m_graph.getAmount() || sink >= m_graph.getAmount()) {
        return allPaths;
    }

    std::map<unsigned int, std::vector<unsigned int>> adj;
    for (const auto &pair: m_graph.getEdges().asKeyValueRange()) {
        unsigned int u = pair.first.first->getIndex();
        unsigned int v = pair.first.second->getIndex();
        adj[u].push_back(v);
    }

    std::vector<unsigned int> currentPath = {source};
    std::set<unsigned int> visited = {source};
    dfsFindPaths(source, sink, currentPath, visited, allPaths, adj);
    return allPaths;
}

void PathFinder::dfsFindPaths(unsigned int current, unsigned int target, std::vector<unsigned int> &currentPath,
                              std::set<unsigned int> &visited, std::vector<Path> &allPaths,
                              const std::map<unsigned int, std::vector<unsigned int>> &adj) {
    if (current == target) {
        Path path;
        path.vertices = currentPath;
        for (size_t i = 0; i < currentPath.size() - 1; ++i) {
            path.edges.emplace_back(currentPath[i], currentPath[i + 1]);
        }
        allPaths.push_back(path);
        return;
    }

    const auto it = adj.find(current);
    if (it == adj.end())
        return;

    for (unsigned int next: it->second) {
        if (!visited.contains(next)) {
            visited.insert(next);
            currentPath.push_back(next);
            dfsFindPaths(next, target, currentPath, visited, allPaths, adj);
            currentPath.pop_back();
            visited.erase(next);
        }
    }
}

std::vector<CutSet> PathFinder::findAllMinimalCuts(unsigned int source, unsigned int sink) {
    auto paths = findAllPaths(source, sink);
    if (paths.empty())
        return {};

    std::set<std::pair<unsigned int, unsigned int>> allEdgesSet;
    for (const auto &pair: m_graph.getEdges().asKeyValueRange()) {
        allEdgesSet.emplace(pair.first.first->getIndex(), pair.first.second->getIndex());
    }

    const std::vector<std::pair<unsigned int, unsigned int>> allEdges(allEdgesSet.begin(), allEdgesSet.end());
    std::vector<CutSet> minimalCuts;
    std::vector<std::pair<unsigned int, unsigned int>> currentCut;
    findMinimalCutsRecursive(allEdges, currentCut, paths, minimalCuts, 0);
    return minimalCuts;
}

void PathFinder::findMinimalCutsRecursive(const std::vector<std::pair<unsigned int, unsigned int>> &allEdges,
                                          std::vector<std::pair<unsigned int, unsigned int>> &currentCut,
                                          const std::vector<Path> &paths, std::vector<CutSet> &minimalCuts,
                                          const size_t startIndex) {
    if (isMinimalCut(currentCut, paths)) {
        bool isDuplicate = false;
        for (const auto &existing: minimalCuts) {
            if (existing.edges.size() == currentCut.size()) {
                std::set<std::pair<unsigned int, unsigned int>> s1(existing.edges.begin(), existing.edges.end());
                std::set<std::pair<unsigned int, unsigned int>> s2(currentCut.begin(), currentCut.end());
                if (s1 == s2) {
                    isDuplicate = true;
                    break;
                }
            }
        }
        if (!isDuplicate)
            minimalCuts.push_back(CutSet{currentCut});
        return;
    }

    for (size_t i = startIndex; i < allEdges.size(); ++i) {
        currentCut.push_back(allEdges[i]);
        findMinimalCutsRecursive(allEdges, currentCut, paths, minimalCuts, i + 1);
        currentCut.pop_back();
    }
}

bool PathFinder::isMinimalCut(const std::vector<std::pair<unsigned int, unsigned int>> &edges,
                              const std::vector<Path> &knownPaths) {
    if (edges.empty())
        return false;
    const std::set<std::pair<unsigned int, unsigned int>> cutSet(edges.begin(), edges.end());

    for (const auto &path: knownPaths) {
        bool pathBlocked = false;
        for (const auto &edge: path.edges) {
            if (cutSet.contains(edge)) {
                pathBlocked = true;
                break;
            }
        }
        if (!pathBlocked)
            return false;
    }
    return true;
}
