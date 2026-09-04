#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class Graph;

struct CandidateEdge {
    int u;
    int v;
    double gain;
    double newReliability;
};

struct DatasetSample {
    int id = 0;
    int numNodes = 0;
    int numEdges = 0;
    int source = 0;
    int sink = 0;
    double baseReliability = 0.0;
    double simulatedGlobalAvailability = 0.0;
    std::pair<int, int> bestEdgeToAdd = {-1, -1};
    double bestGain = 0.0;
    double bestReliability = 0.0;

    struct NodeInfo {
        int id = 0;
        int degree = 0;
        double expRate = 0.1;
        double normMean = 10.0;
        double normStd = 2.0;
        double availability = 1.0;
        double settledReliability = 1.0;
        int failureCount = 0;
        int maintenanceCount = 0;
        int strategyId = 0;
        std::string strategy;
    };
    std::vector<NodeInfo> nodes;

    struct EdgeInfo {
        int u;
        int v;
        double weight;
        double bandwidth;
        double flow;
        double loadRatio;
        double reliability;
    };
    std::vector<EdgeInfo> edges;

    std::vector<std::vector<double>> adjMatrix;
    std::vector<std::vector<double>> bandwidthMatrix;
    std::vector<std::vector<double>> flowMatrix;

    std::vector<CandidateEdge> rankedCandidates;

    QJsonObject toJson() const;
};

class DatasetGenerator {
public:
    explicit DatasetGenerator(int warmupSteps = 150);

    void setWarmupSteps(int steps) { m_warmupSteps = steps; }
    int warmupSteps() const { return m_warmupSteps; }

    DatasetSample generateRandomSample(int id, int numNodes);
    DatasetSample sampleFromGraph(const Graph &graph, int source = 0, int sink = -1);

    QJsonObject generateDataset(int numSamples, int minNodes = 10, int maxNodes = 20,
                                std::function<void(int, int)> progressCb = nullptr);

    bool saveDatasetToFile(const QString &filePath, int numSamples, int minNodes = 10, int maxNodes = 20,
                           std::function<void(int, int)> progressCb = nullptr);

    bool saveCurrentGraphToFile(const QString &filePath, const Graph &graph, int source = 0, int sink = -1);

private:
    int m_warmupSteps;

    double computeTwoTerminalReliability(int numNodes,
                                        const std::vector<std::vector<int>> &adj,
                                        const std::vector<std::vector<double>> &edgeRel,
                                        int source, int sink);

    void dfsPaths(int curr, int sink, const std::vector<std::vector<int>> &adj,
                  std::vector<int> &currentPath, std::vector<bool> &visited,
                  std::vector<std::vector<int>> &allPaths, int maxPaths = 150);
};
