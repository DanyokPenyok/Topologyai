#include "datasetgenerator.h"
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <algorithm>
#include <cmath>
#include <random>
#include "graph.h"
#include "node.h"
#include "nodemodel.h"
#include "simulationengine.h"
#include "imaintenancestrategy.h"
#include "strategybasiccontrol.h"
#include "strategyfixedintervalcontrol.h"
#include "strategyinstantcheckdetection.h"
#include "strategyinstantdetection.h"
#include "strategynocontrolcheck.h"
#include "strategyofflinecheck.h"
#include "strategyperiodiccontrolemergencyrecovery.h"
#include "strategypreventivewithcontrol.h"

static std::unique_ptr<IMaintenanceStrategy> createStrategyByIndex(int type) {
    switch (type % 8) {
        case 0: return std::make_unique<StrategyBasicControl>();
        case 1: return std::make_unique<StrategyInstantDetection>();
        case 2: return std::make_unique<StrategyFixedIntervalControl>(10.0);
        case 3: return std::make_unique<StrategyPreventiveWithControl>(12.0);
        case 4: return std::make_unique<StrategyNoControlCheck>(8.0, 1.0);
        case 5: return std::make_unique<StrategyOfflineCheck>(6.0, 1.0);
        case 6: return std::make_unique<StrategyPeriodicControlEmergencyRecovery>(15.0);
        case 7: return std::make_unique<StrategyInstantCheckDetection>(10.0, 1.5);
        default: return std::make_unique<StrategyBasicControl>();
    }
}

static int getStrategyIdByName(const std::string &name) {
    if (name.find("1.1") != std::string::npos || name.find("без локализации") != std::string::npos) return 0;
    if (name.find("1.2") != std::string::npos || name.find("Мгновенное обнаружение") != std::string::npos) return 1;
    if (name.find("3.") != std::string::npos || name.find("фиксированная") != std::string::npos) return 2;
    if (name.find("2.1") != std::string::npos || name.find("Профилактика + встроенный") != std::string::npos) return 3;
    if (name.find("2.2") != std::string::npos || name.find("без контроля, без отключения") != std::string::npos) return 4;
    if (name.find("2.4") != std::string::npos || name.find("с отключением при проверке") != std::string::npos) return 5;
    if (name.find("4.") != std::string::npos || name.find("аварийная профилактика") != std::string::npos) return 6;
    if (name.find("2.3") != std::string::npos || name.find("мгновенное обнаружение при проверке") != std::string::npos) return 7;
    return 0;
}

QJsonObject DatasetSample::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["num_nodes"] = numNodes;
    obj["num_edges"] = numEdges;
    obj["source"] = source;
    obj["sink"] = sink;
    obj["base_reliability"] = baseReliability;
    obj["simulated_global_availability"] = simulatedGlobalAvailability;

    QJsonArray bestEdgeArr;
    bestEdgeArr.append(bestEdgeToAdd.first);
    bestEdgeArr.append(bestEdgeToAdd.second);
    obj["target_best_edge"] = bestEdgeArr;
    obj["target_gain"] = bestGain;
    obj["target_reliability"] = bestReliability;

    QJsonArray nodesArr;
    for (const auto &n : nodes) {
        QJsonObject nobj;
        nobj["id"] = n.id;
        nobj["degree"] = n.degree;
        nobj["exp_rate"] = n.expRate;
        nobj["norm_mean"] = n.normMean;
        nobj["norm_std"] = n.normStd;
        nobj["availability"] = n.availability;
        nobj["settled_reliability"] = n.settledReliability;
        nobj["failures"] = n.failureCount;
        nobj["maintenances"] = n.maintenanceCount;
        nobj["strategy_id"] = n.strategyId;

        QJsonArray oneHot;
        for (int s = 0; s < 8; ++s) {
            oneHot.append(s == n.strategyId ? 1 : 0);
        }
        nobj["strategy_one_hot"] = oneHot;
        nobj["strategy"] = QString::fromStdString(n.strategy);
        nodesArr.append(nobj);
    }
    obj["nodes"] = nodesArr;

    QJsonArray edgesArr;
    for (const auto &e : edges) {
        QJsonObject eobj;
        eobj["u"] = e.u;
        eobj["v"] = e.v;
        eobj["weight"] = e.weight;
        eobj["bandwidth"] = e.bandwidth;
        eobj["flow"] = e.flow;
        eobj["load_ratio"] = e.loadRatio;
        eobj["reliability"] = e.reliability;
        edgesArr.append(eobj);
    }
    obj["edges"] = edgesArr;

    QJsonArray adjArr;
    for (const auto &row : adjMatrix) {
        QJsonArray r;
        for (double val : row) {
            r.append(val);
        }
        adjArr.append(r);
    }
    obj["adjacency_matrix"] = adjArr;

    QJsonArray bandArr;
    for (const auto &row : bandwidthMatrix) {
        QJsonArray r;
        for (double val : row) {
            r.append(val);
        }
        bandArr.append(r);
    }
    obj["bandwidth_matrix"] = bandArr;

    QJsonArray flowArr;
    for (const auto &row : flowMatrix) {
        QJsonArray r;
        for (double val : row) {
            r.append(val);
        }
        flowArr.append(r);
    }
    obj["flow_matrix"] = flowArr;

    QJsonArray candArr;
    size_t limit = std::min<size_t>(rankedCandidates.size(), 30);
    for (size_t i = 0; i < limit; ++i) {
        QJsonObject cobj;
        cobj["u"] = rankedCandidates[i].u;
        cobj["v"] = rankedCandidates[i].v;
        cobj["gain"] = rankedCandidates[i].gain;
        cobj["new_reliability"] = rankedCandidates[i].newReliability;
        candArr.append(cobj);
    }
    obj["candidate_edges_ranked"] = candArr;

    return obj;
}

DatasetGenerator::DatasetGenerator(int warmupSteps) : m_warmupSteps(warmupSteps) {}

void DatasetGenerator::dfsPaths(int curr, int sink, const std::vector<std::vector<int>> &adj,
                               std::vector<int> &currentPath, std::vector<bool> &visited,
                               std::vector<std::vector<int>> &allPaths, int maxPaths) {
    if (allPaths.size() >= static_cast<size_t>(maxPaths))
        return;

    if (curr == sink) {
        allPaths.push_back(currentPath);
        return;
    }

    for (int next : adj[curr]) {
        if (!visited[next]) {
            visited[next] = true;
            currentPath.push_back(next);
            dfsPaths(next, sink, adj, currentPath, visited, allPaths, maxPaths);
            currentPath.pop_back();
            visited[next] = false;
        }
    }
}

double DatasetGenerator::computeTwoTerminalReliability(int numNodes,
                                                     const std::vector<std::vector<int>> &adj,
                                                     const std::vector<std::vector<double>> &edgeRel,
                                                     int source, int sink) {
    if (source < 0 || sink < 0 || source >= numNodes || sink >= numNodes || source == sink)
        return 0.0;

    std::vector<std::vector<int>> paths;
    std::vector<int> currentPath = {source};
    std::vector<bool> visited(numNodes, false);
    visited[source] = true;

    dfsPaths(source, sink, adj, currentPath, visited, paths, 150);

    if (paths.empty())
        return 0.0;

    double reliability = 0.0;
    for (const auto &path : paths) {
        double pathProb = 1.0;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            int u = path[i];
            int v = path[i + 1];
            pathProb *= edgeRel[u][v];
        }
        reliability = 1.0 - (1.0 - reliability) * (1.0 - pathProb);
    }

    return reliability;
}

DatasetSample DatasetGenerator::generateRandomSample(int id, int numNodes) {
    DatasetSample sample;
    sample.id = id;
    sample.numNodes = numNodes;
    sample.source = 0;
    sample.sink = numNodes - 1;

    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> distExp(0.05, 0.25);
    std::uniform_real_distribution<double> distNormMean(6.0, 14.0);
    std::uniform_real_distribution<double> distNormStd(1.0, 2.5);
    std::uniform_real_distribution<double> distWeight(1.0, 10.0);
    std::uniform_real_distribution<double> distFlowRatio(0.2, 0.85);

    const double bandwidthOptions[] = {100.0, 1000.0, 10000.0};
    std::uniform_int_distribution<int> distBand(0, 2);

    std::vector<std::shared_ptr<NodeModel>> nodeModels(numNodes);
    for (int i = 0; i < numNodes; ++i) {
        nodeModels[i] = std::make_shared<NodeModel>(i);
        nodeModels[i]->setExpRate(distExp(gen));
        nodeModels[i]->setNormalParams(distNormMean(gen), distNormStd(gen));
        nodeModels[i]->setStrategy(createStrategyByIndex(i + id));
    }

    // Прогрев симуляции
    SimulationEngine simEngine(0.1, "exponential");
    simEngine.setSilent(true);
    simEngine.setNodes(nodeModels);
    simEngine.start();
    for (int step = 0; step < m_warmupSteps; ++step) {
        simEngine.step();
    }
    simEngine.stop();

    sample.simulatedGlobalAvailability = simEngine.getGlobalAvailability();

    std::vector<std::vector<int>> adjList(numNodes);
    std::vector<std::vector<double>> edgeRel(numNodes, std::vector<double>(numNodes, 0.0));
    sample.adjMatrix.assign(numNodes, std::vector<double>(numNodes, 0.0));
    sample.bandwidthMatrix.assign(numNodes, std::vector<double>(numNodes, 0.0));
    sample.flowMatrix.assign(numNodes, std::vector<double>(numNodes, 0.0));

    // Остовное дерево
    for (int i = 1; i < numNodes; ++i) {
        std::uniform_int_distribution<int> distParent(0, i - 1);
        int parent = distParent(gen);

        double w = std::round(distWeight(gen) * 10.0) / 10.0;
        double b = bandwidthOptions[distBand(gen)];
        double f = std::round(b * distFlowRatio(gen) * 10.0) / 10.0;
        double load = (b > 0.0) ? (f / b) : 0.0;

        double k_parent = simEngine.getNodeAvailability(parent);
        double k_child = simEngine.getNodeAvailability(i);
        double r = std::clamp(k_parent * k_child * (0.98 - 0.05 * load), 0.40, 0.99);

        adjList[parent].push_back(i);
        adjList[i].push_back(parent);

        edgeRel[parent][i] = edgeRel[i][parent] = r;
        sample.adjMatrix[parent][i] = sample.adjMatrix[i][parent] = w;
        sample.bandwidthMatrix[parent][i] = sample.bandwidthMatrix[i][parent] = b;
        sample.flowMatrix[parent][i] = sample.flowMatrix[i][parent] = f;

        sample.edges.push_back({parent, i, w, b, f, load, r});
    }

    // Дополнительные хорды
    std::uniform_int_distribution<int> distExtraCount(0, 3);
    int extraEdges = distExtraCount(gen);
    std::uniform_int_distribution<int> distNode(0, numNodes - 1);

    for (int k = 0; k < extraEdges; ++k) {
        int u = distNode(gen);
        int v = distNode(gen);
        if (u != v && sample.adjMatrix[u][v] == 0.0) {
            double w = std::round(distWeight(gen) * 10.0) / 10.0;
            double b = bandwidthOptions[distBand(gen)];
            double f = std::round(b * distFlowRatio(gen) * 10.0) / 10.0;
            double load = (b > 0.0) ? (f / b) : 0.0;

            double ku = simEngine.getNodeAvailability(u);
            double kv = simEngine.getNodeAvailability(v);
            double r = std::clamp(ku * kv * (0.98 - 0.05 * load), 0.40, 0.99);

            adjList[u].push_back(v);
            adjList[v].push_back(u);

            edgeRel[u][v] = edgeRel[v][u] = r;
            sample.adjMatrix[u][v] = sample.adjMatrix[v][u] = w;
            sample.bandwidthMatrix[u][v] = sample.bandwidthMatrix[v][u] = b;
            sample.flowMatrix[u][v] = sample.flowMatrix[v][u] = f;

            sample.edges.push_back({u, v, w, b, f, load, r});
        }
    }

    sample.numEdges = static_cast<int>(sample.edges.size());

    auto finalStats = simEngine.getFinalStats();
    for (int i = 0; i < numNodes; ++i) {
        const auto &dp = nodeModels[i]->getDistributionParams();
        double avail = simEngine.getNodeAvailability(i);
        double settledRel = nodeModels[i]->reliability();

        int failures = (i < static_cast<int>(finalStats.size())) ? finalStats[i].failureCount : 0;
        int maints = (i < static_cast<int>(finalStats.size())) ? finalStats[i].maintenanceCount : 0;
        std::string stratName = nodeModels[i]->strategy() ? nodeModels[i]->strategy()->name() : "None";
        int stratId = getStrategyIdByName(stratName);

        sample.nodes.push_back({
            i,
            static_cast<int>(adjList[i].size()),
            dp.expRate,
            dp.normMean,
            dp.normStd,
            avail,
            settledRel,
            failures,
            maints,
            stratId,
            stratName
        });
    }

    sample.baseReliability = computeTwoTerminalReliability(numNodes, adjList, edgeRel, sample.source, sample.sink);

    // Оценка кандидатов
    for (int u = 0; u < numNodes; ++u) {
        for (int v = u + 1; v < numNodes; ++v) {
            if (sample.adjMatrix[u][v] == 0.0) {
                double ku = simEngine.getNodeAvailability(u);
                double kv = simEngine.getNodeAvailability(v);
                double testNewEdgeRel = std::clamp(ku * kv * 0.96, 0.40, 0.99);

                adjList[u].push_back(v);
                adjList[v].push_back(u);
                edgeRel[u][v] = edgeRel[v][u] = testNewEdgeRel;

                double newRel = computeTwoTerminalReliability(numNodes, adjList, edgeRel, sample.source, sample.sink);
                double gain = newRel - sample.baseReliability;

                if (gain > 0.0001) {
                    sample.rankedCandidates.push_back({u, v, gain, newRel});
                }

                adjList[u].pop_back();
                adjList[v].pop_back();
                edgeRel[u][v] = edgeRel[v][u] = 0.0;
            }
        }
    }

    std::sort(sample.rankedCandidates.begin(), sample.rankedCandidates.end(),
              [](const CandidateEdge &a, const CandidateEdge &b) {
                  return a.gain > b.gain;
              });

    if (!sample.rankedCandidates.empty()) {
        sample.bestEdgeToAdd = {sample.rankedCandidates[0].u, sample.rankedCandidates[0].v};
        sample.bestGain = sample.rankedCandidates[0].gain;
        sample.bestReliability = sample.rankedCandidates[0].newReliability;
    } else {
        sample.bestEdgeToAdd = {-1, -1};
        sample.bestGain = 0.0;
        sample.bestReliability = sample.baseReliability;
    }

    return sample;
}

DatasetSample DatasetGenerator::sampleFromGraph(const Graph &graph, int source, int sink) {
    DatasetSample sample;
    int n = static_cast<int>(graph.getAmount());
    sample.id = 0;
    sample.numNodes = n;
    sample.source = (source >= 0 && source < n) ? source : 0;
    sample.sink = (sink >= 0 && sink < n && sink != sample.source) ? sink : (n > 1 ? n - 1 : 0);

    const auto &matrixAdj = graph.getMatrixAdjacent();
    const auto &matrixBand = graph.getMatrixBandwidth();
    const auto &matrixFlow = graph.getMatrixFlow();
    const auto &nodesMap = graph.getNodes();

    std::vector<std::shared_ptr<NodeModel>> nodeModels(n);
    for (int i = 0; i < n; ++i) {
        nodeModels[i] = std::make_shared<NodeModel>(i);
        if (nodesMap.contains(i) && nodesMap[i]->model()) {
            const auto *orig = nodesMap[i]->model();
            nodeModels[i]->setDistributionParams(orig->getDistributionParams());
            nodeModels[i]->setStrategy(createStrategyByIndex(i));
        } else {
            nodeModels[i]->setStrategy(createStrategyByIndex(i));
        }
    }

    // Прогрев симуляции
    SimulationEngine simEngine(0.1, "exponential");
    simEngine.setSilent(true);
    simEngine.setNodes(nodeModels);
    simEngine.start();
    for (int step = 0; step < m_warmupSteps; ++step) {
        simEngine.step();
    }
    simEngine.stop();

    sample.simulatedGlobalAvailability = simEngine.getGlobalAvailability();

    std::vector<std::vector<int>> adjList(n);
    std::vector<std::vector<double>> edgeRel(n, std::vector<double>(n, 0.0));
    sample.adjMatrix.assign(n, std::vector<double>(n, 0.0));
    sample.bandwidthMatrix.assign(n, std::vector<double>(n, 0.0));
    sample.flowMatrix.assign(n, std::vector<double>(n, 0.0));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double w = (i < matrixAdj.size() && j < matrixAdj[i].size()) ? matrixAdj[i][j] : 0.0;
            double b = (i < matrixBand.size() && j < matrixBand[i].size()) ? matrixBand[i][j] : 0.0;
            double f = (i < matrixFlow.size() && j < matrixFlow[i].size()) ? matrixFlow[i][j] : 0.0;

            sample.adjMatrix[i][j] = w;
            sample.bandwidthMatrix[i][j] = b;
            sample.flowMatrix[i][j] = f;
        }
    }

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            double w = std::max(sample.adjMatrix[i][j], sample.adjMatrix[j][i]);
            double b = std::max(sample.bandwidthMatrix[i][j], sample.bandwidthMatrix[j][i]);
            double f = std::max(sample.flowMatrix[i][j], sample.flowMatrix[j][i]);

            if (w > 0.0 || b > 0.0) {
                adjList[i].push_back(j);
                adjList[j].push_back(i);

                double ki = simEngine.getNodeAvailability(i);
                double kj = simEngine.getNodeAvailability(j);
                double load = (b > 0.0) ? (f / b) : 0.0;
                double loadPenalty = load * 0.05;
                double rel = std::clamp(ki * kj * (0.98 - loadPenalty), 0.40, 0.99);

                edgeRel[i][j] = edgeRel[j][i] = rel;
                sample.edges.push_back({i, j, std::max(1.0, w), std::max(1.0, b), f, load, rel});
            }
        }
    }

    sample.numEdges = static_cast<int>(sample.edges.size());

    auto finalStats = simEngine.getFinalStats();
    for (int i = 0; i < n; ++i) {
        const auto &params = nodeModels[i]->getDistributionParams();
        double avail = simEngine.getNodeAvailability(i);
        double settledRel = nodeModels[i]->reliability();
        int failures = (i < static_cast<int>(finalStats.size())) ? finalStats[i].failureCount : 0;
        int maints = (i < static_cast<int>(finalStats.size())) ? finalStats[i].maintenanceCount : 0;
        std::string strat = nodeModels[i]->strategy() ? nodeModels[i]->strategy()->name() : "None";
        int stratId = getStrategyIdByName(strat);

        sample.nodes.push_back({
            i,
            static_cast<int>(adjList[i].size()),
            params.expRate,
            params.normMean,
            params.normStd,
            avail,
            settledRel,
            failures,
            maints,
            stratId,
            strat
        });
    }

    sample.baseReliability = computeTwoTerminalReliability(n, adjList, edgeRel, sample.source, sample.sink);

    // Оценка кандидатов
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (sample.adjMatrix[u][v] == 0.0 && sample.adjMatrix[v][u] == 0.0 &&
                sample.bandwidthMatrix[u][v] == 0.0 && sample.bandwidthMatrix[v][u] == 0.0) {
                double ku = simEngine.getNodeAvailability(u);
                double kv = simEngine.getNodeAvailability(v);
                double testNewEdgeRel = std::clamp(ku * kv * 0.96, 0.40, 0.99);

                adjList[u].push_back(v);
                adjList[v].push_back(u);
                edgeRel[u][v] = edgeRel[v][u] = testNewEdgeRel;

                double newRel = computeTwoTerminalReliability(n, adjList, edgeRel, sample.source, sample.sink);
                double gain = newRel - sample.baseReliability;

                if (gain > 0.0001) {
                    sample.rankedCandidates.push_back({u, v, gain, newRel});
                }

                adjList[u].pop_back();
                adjList[v].pop_back();
                edgeRel[u][v] = edgeRel[v][u] = 0.0;
            }
        }
    }

    std::sort(sample.rankedCandidates.begin(), sample.rankedCandidates.end(),
              [](const CandidateEdge &a, const CandidateEdge &b) {
                  return a.gain > b.gain;
              });

    if (!sample.rankedCandidates.empty()) {
        sample.bestEdgeToAdd = {sample.rankedCandidates[0].u, sample.rankedCandidates[0].v};
        sample.bestGain = sample.rankedCandidates[0].gain;
        sample.bestReliability = sample.rankedCandidates[0].newReliability;
    } else {
        sample.bestEdgeToAdd = {-1, -1};
        sample.bestGain = 0.0;
        sample.bestReliability = sample.baseReliability;
    }

    return sample;
}

QJsonObject DatasetGenerator::generateDataset(int numSamples, int minNodes, int maxNodes,
                                             std::function<void(int, int)> progressCb) {
    QJsonObject root;
    root["description"] = "Synthetic dataset for network reliability optimization";
    root["generated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["samples_count"] = numSamples;
    root["warmup_steps"] = m_warmupSteps;

    QJsonObject stratMap;
    stratMap["0"] = "1.1 Контроль без локализации отказа";
    stratMap["1"] = "1.2 Мгновенное обнаружение отказа";
    stratMap["2"] = "3. Встроенный контроль + фиксированная профилактика";
    stratMap["3"] = "2.1 Профилактика + встроенный контроль";
    stratMap["4"] = "2.2 Профилактика без контроля, без отключения";
    stratMap["5"] = "2.4 Профилактика с отключением при проверке";
    stratMap["6"] = "4. Периодический контроль + аварийная профилактика";
    stratMap["7"] = "2.3 Профилактика, мгновенное обнаружение при проверке";
    root["strategy_mapping"] = stratMap;

    QJsonArray samplesArray;
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> distN(std::max(3, minNodes), std::max(minNodes, maxNodes));

    for (int i = 0; i < numSamples; ++i) {
        int n = distN(gen);
        DatasetSample sample = generateRandomSample(i, n);
        samplesArray.append(sample.toJson());

        if (progressCb) {
            progressCb(i + 1, numSamples);
        }
    }

    root["samples"] = samplesArray;
    return root;
}

bool DatasetGenerator::saveDatasetToFile(const QString &filePath, int numSamples, int minNodes, int maxNodes,
                                        std::function<void(int, int)> progressCb) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QJsonObject root = generateDataset(numSamples, minNodes, maxNodes, progressCb);
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool DatasetGenerator::saveCurrentGraphToFile(const QString &filePath, const Graph &graph, int source, int sink) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    DatasetSample sample = sampleFromGraph(graph, source, sink);
    QJsonObject root;
    root["description"] = "Exported network state";
    root["exported_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonObject stratMap;
    stratMap["0"] = "1.1 Контроль без локализации отказа";
    stratMap["1"] = "1.2 Мгновенное обнаружение отказа";
    stratMap["2"] = "3. Встроенный контроль + фиксированная профилактика";
    stratMap["3"] = "2.1 Профилактика + встроенный контроль";
    stratMap["4"] = "2.2 Профилактика без контроля, без отключения";
    stratMap["5"] = "2.4 Профилактика с отключением при проверке";
    stratMap["6"] = "4. Периодический контроль + аварийная профилактика";
    stratMap["7"] = "2.3 Профилактика, мгновенное обнаружение при проверке";
    root["strategy_mapping"] = stratMap;

    root["sample"] = sample.toJson();

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
