#include "networkanalyzer.h"
#include <sstream>

NetworkAnalyzer::NetworkAnalyzer(const Graph& graph)
    : m_graph(graph), m_pathFinder(graph) {}

NetworkAnalysisResult NetworkAnalyzer::analyze(unsigned int source, unsigned int sink) {
    NetworkAnalysisResult result;

    result.minimalPaths = m_pathFinder.findAllPaths(source, sink);
    result.minimalPaths = LogicalForm::simplifyPaths(result.minimalPaths);

    result.minimalCuts = m_pathFinder.findAllMinimalCuts(source, sink);

    result.dnf = LogicalForm::buildDNF(result.minimalPaths);
    result.cnf = LogicalForm::buildCNF(result.minimalCuts);

    return result;
}

std::string NetworkAnalyzer::getAnalysisReport(const NetworkAnalysisResult& result) const {
    std::ostringstream oss;
    oss << "=== СТРУКТУРНЫЙ АНАЛИЗ СЕТИ ===\n\n";
    oss << "Найдено минимальных путей: " << result.minimalPaths.size() << "\n";
    oss << "Найдено минимальных сечений: " << result.minimalCuts.size() << "\n\n";

    oss << "ДНФ:\n" << result.dnf << "\n\n";
    oss << "КНФ:\n" << result.cnf << "\n";

    if (!result.minimalCuts.empty() && result.minimalCuts.front().edges.size() == 1) {
        oss << "\n⚠ Внимание: обнаружено сечение из одного ребра — критическая точка отказа!\n";
    }

    return oss.str();
}