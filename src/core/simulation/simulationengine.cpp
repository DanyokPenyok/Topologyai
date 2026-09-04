#include "simulationengine.h"

#include <QFile>
#include <algorithm>
#include <random>
#include "logger.h"

SimulationEngine::SimulationEngine(double dt, const std::string &distMode) :
    m_dt(dt), m_currentTime(0.0), m_running(false), m_totalRuntime(0.0), m_totalDowntime(0.0), m_emulator(distMode) {}

void SimulationEngine::setNodes(const std::vector<std::shared_ptr<NodeModel>> &nodes) {
    m_nodes = nodes;
    std::map<unsigned int, bool> activeNodeIds;
    for (const auto &node: m_nodes) {
        activeNodeIds[node->id()] = true;
        if (!m_nodeStats.contains(node->id())) {
            m_nodeStats[node->id()] = NodeStats{};
        }
    }

    for (auto it = m_nodeStats.begin(); it != m_nodeStats.end();) {
        if (!activeNodeIds.contains(it->first)) {
            it = m_nodeStats.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_maintenanceEndTimes.begin(); it != m_maintenanceEndTimes.end();) {
        if (!activeNodeIds.contains(it->first)) {
            it = m_maintenanceEndTimes.erase(it);
        } else {
            ++it;
        }
    }
    if (!m_silent) {
        Logger::info(QString("Инициализировано %1 узлов").arg(nodes.size()));
    }
}

void SimulationEngine::start() {
    if (!m_running && !m_nodes.empty()) {
        m_running = true;
    }
}

void SimulationEngine::stop() { m_running = false; }

bool SimulationEngine::isRunning() const { return m_running; }

void SimulationEngine::reset() {
    m_currentTime = 0.0;
    m_totalRuntime = 0.0;
    m_totalDowntime = 0.0;
    m_nodeStats.clear();
    m_maintenanceEndTimes.clear();
    m_reliabilityHistory.clear();
    for (const auto &node: m_nodes) {
        node->setState(NodeModel::State::Operational);
        node->setReliability(1.0);
        node->setNextFailureTime(0.0);
        m_nodeStats[node->id()] = NodeStats{};
    }
}

double SimulationEngine::currentTime() const { return m_currentTime; }

double SimulationEngine::getGlobalAvailability() const {
    double total = m_totalRuntime + m_totalDowntime;
    return (total > 0.0) ? m_totalRuntime / total : 1.0;
}

double SimulationEngine::getNodeAvailability(unsigned int nodeId) const {
    auto it = m_nodeStats.find(nodeId);
    if (it == m_nodeStats.end())
        return 1.0;

    const auto &stats = it->second;
    double total = stats.runtime + stats.downtime;
    return (total > 0.0) ? stats.runtime / total : 1.0;
}

void SimulationEngine::setStepCallback(StepCallback cb) { m_stepCallback = std::move(cb); }

void SimulationEngine::setEventCallback(EventCallback cb) { m_eventCallback = std::move(cb); }

MessageBus &SimulationEngine::messageBus() { return m_bus; }

void SimulationEngine::step() {
    if (!m_running || m_nodes.empty()) {
        return;
    }

    for (auto &node: m_nodes) {
        node->clearInbox();
        auto msgs = m_bus.fetchFor(node->id());
        for (const auto &msg: msgs) {
            node->addMessage(msg);
        }
    }

    for (auto &node: m_nodes) {
        processNode(*node);
    }

    m_bus.flush();

    m_currentTime += m_dt;

    if (m_recordHistory) {
        m_reliabilityHistory.push_back({m_currentTime, getGlobalAvailability()});
    }

    if (!m_silent) {
        Logger::step(m_currentTime, getGlobalAvailability());
    }

    if (m_stepCallback) {
        m_stepCallback();
    }
}

std::vector<SimulationEngine::NodeFinalStats> SimulationEngine::getFinalStats() const {
    std::vector<NodeFinalStats> stats;
    stats.reserve(m_nodes.size());

    for (const auto &node: m_nodes) {
        NodeFinalStats s;
        s.id = node->id();

        auto it = m_nodeStats.find(node->id());
        if (it != m_nodeStats.end()) {
            const auto &ns = it->second;
            double total = ns.runtime + ns.downtime;
            s.availabilityPercent = (total > 0.0) ? (ns.runtime / total * 100.0) : 100.0;
            s.totalRuntime = ns.runtime;
            s.totalDowntime = ns.downtime;
            s.failureCount = ns.failureCount;
            s.maintenanceCount = ns.maintenanceCount;
        } else {
            s.availabilityPercent = 100.0;
            s.totalRuntime = 0.0;
            s.totalDowntime = 0.0;
            s.failureCount = 0;
            s.maintenanceCount = 0;
        }

        stats.push_back(s);
    }

    return stats;
}

bool SimulationEngine::exportStatsToCSV(const QString &fileName) const {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    // Заголовок с кодировкой UTF-8 для корректного отображения кириллицы
    out << "\xEF\xBB\xBF"; // BOM для UTF-8
    out << "NodeID;Availability(%);Runtime(s);Downtime(s);Failures;Maintenances\n";

    // Получаем статистику
    auto stats = getFinalStats();

    double totalAvailability = 0.0;
    unsigned int totalFailures = 0;
    unsigned int totalMaintenances = 0;

    // Записываем данные по каждому узлу
    for (const auto &s: stats) {
        out << s.id << ";" << QString::number(s.availabilityPercent, 'f', 2) << ";"
            << QString::number(s.totalRuntime, 'f', 2) << ";" << QString::number(s.totalDowntime, 'f', 2) << ";"
            << s.failureCount << ";" << s.maintenanceCount << "\n";

        totalAvailability += s.availabilityPercent;
        totalFailures += s.failureCount;
        totalMaintenances += s.maintenanceCount;
    }

    // Добавляем строку с усреднёнными данными
    if (!stats.empty()) {
        double avgAvailability = totalAvailability / stats.size();
        out << "\n# Сводные данные по сети:\n";
        out << "AVERAGE_AVAILABILITY;" << QString::number(avgAvailability, 'f', 2) << "%\n";
        out << "TOTAL_FAILURES;" << totalFailures << "\n";
        out << "TOTAL_MAINTENANCES;" << totalMaintenances << "\n";
        out << "NODES_COUNT;" << stats.size() << "\n";
    }

    file.close();

    // Проверяем, что файл действительно записан
    if (file.size() > 0) {
        Logger::info(QString("Результаты сохранены: %1 (%2 байт)").arg(fileName).arg(file.size()));
        return true;
    }

    return false;
}

void SimulationEngine::processNode(NodeModel &node) {
    auto *strategy = node.strategy();
    if (!strategy) {

        m_totalRuntime += m_dt;
        auto &stats = m_nodeStats[node.id()];
        stats.runtime += m_dt;
        return;
    }

    const auto &distParams = node.getDistributionParams();

    const auto state = node.state();
    if (state != NodeModel::State::Operational) {
        m_totalDowntime += m_dt;
        auto &stats = m_nodeStats[node.id()];
        stats.downtime += m_dt;

        if (state == NodeModel::State::Failed) {
            handleRecovery(node);
            const double nextFailure = m_emulator.sampleFailureTime(distParams.type, distParams.expRate,
                                                                    distParams.normMean, distParams.normStd);
            node.setNextFailureTime(m_currentTime + nextFailure);
        } else if (state == NodeModel::State::Maintenance) {
            auto it = m_maintenanceEndTimes.find(node.id());
            if (it != m_maintenanceEndTimes.end() && m_currentTime >= it->second) {
                node.setState(NodeModel::State::Operational);
                node.setReliability(1.0);
                m_bus.sendTo(node.id(), "MAINT_COMPLETE");
                if (m_eventCallback)
                    m_eventCallback(node.id(), "MAINT_COMPLETE");
                m_maintenanceEndTimes.erase(it);

                const double nextFailure = m_emulator.sampleFailureTime(distParams.type, distParams.expRate,
                                                                        distParams.normMean, distParams.normStd);
                node.setNextFailureTime(m_currentTime + nextFailure);
            }
        }
        return;
    }

    if (node.getNextFailureTime() <= 0.0) {
        const double nextFailure = m_emulator.sampleFailureTime(distParams.type, distParams.expRate,
                                                                distParams.normMean, distParams.normStd);
        node.setNextFailureTime(m_currentTime + nextFailure);
        return;
    }

    if (m_currentTime >= node.getNextFailureTime()) {
        triggerFailure(node);
        return;
    }

    if (strategy->checkMaintenance(m_currentTime)) {
        const double maintDuration = m_emulator.sampleMaintenanceTime(distParams.type, distParams.expRate,
                                                                      distParams.normMean, distParams.normStd);
        m_maintenanceEndTimes[node.id()] = m_currentTime + maintDuration;
        node.setState(NodeModel::State::Maintenance);
        auto &stats = m_nodeStats[node.id()];
        stats.maintenanceCount++;
        m_bus.sendTo(node.id(), "MAINT_START");
        if (m_eventCallback)
            m_eventCallback(node.id(), "MAINTENANCE");
        return;
    }
    m_totalRuntime += m_dt;
    auto &stats = m_nodeStats[node.id()];
    stats.runtime += m_dt;

    double newReliability = node.reliability() * std::exp(-m_dt / 100.0);
    node.setReliability(std::max(0.0, newReliability));
}

void SimulationEngine::triggerFailure(NodeModel &node) {
    auto *strategy = node.strategy();
    if (!strategy)
        return;

    node.setState(NodeModel::State::Failed);
    auto &stats = m_nodeStats[node.id()];
    stats.failureCount++;

    double detectTime = strategy->detectFailure();
    double recoverTime = strategy->recover();
    double totalDowntime = detectTime + recoverTime;

    m_totalDowntime += std::min(totalDowntime, m_dt);
    stats.downtime += std::min(totalDowntime, m_dt);

    m_bus.sendTo(node.id(), "FAILURE_DETECTED");
    if (m_eventCallback) {
        m_eventCallback(node.id(), "FAILURE");
    }
}

void SimulationEngine::handleRecovery(NodeModel &node) {
    node.setState(NodeModel::State::Operational);
    node.setReliability(0.95);

    m_bus.sendTo(node.id(), "RECOVERED");
    if (m_eventCallback) {
        m_eventCallback(node.id(), "RECOVERY");
    }
}
