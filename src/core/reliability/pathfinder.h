#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

class Graph;

//! @brief Представляет путь в графе
struct Path {
    std::vector<unsigned int> vertices;
    std::vector<std::pair<unsigned int, unsigned int>> edges;

    bool operator<(const Path &other) const { return edges < other.edges; }
    std::string toString() const;
};

//! @brief Представляет минимальное сечение
struct CutSet {
    std::vector<std::pair<unsigned int, unsigned int>> edges;

    bool operator<(const CutSet &other) const { return edges < other.edges; }
    std::string toString() const;
};

//! @brief Поиск путей и сечений в графе без использования матрицы смежности
class PathFinder {
public:
    explicit PathFinder(const Graph &graph);

    std::vector<Path> findAllPaths(unsigned int source, unsigned int sink);
    std::vector<CutSet> findAllMinimalCuts(unsigned int source, unsigned int sink);

private:
    const Graph &m_graph;

    void dfsFindPaths(unsigned int current, unsigned int target, std::vector<unsigned int> &currentPath,
                      std::set<unsigned int> &visited, std::vector<Path> &allPaths,
                      const std::map<unsigned int, std::vector<unsigned int>> &adj);

    void findMinimalCutsRecursive(const std::vector<std::pair<unsigned int, unsigned int>> &allEdges,
                                  std::vector<std::pair<unsigned int, unsigned int>> &currentCut,
                                  const std::vector<Path> &paths, std::vector<CutSet> &minimalCuts, size_t startIndex);

    bool isMinimalCut(const std::vector<std::pair<unsigned int, unsigned int>> &edges,
                      const std::vector<Path> &knownPaths);
};
