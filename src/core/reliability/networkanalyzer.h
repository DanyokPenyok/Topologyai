#pragma once

#include <string>
#include <vector>
#include "logicalforms.h"
#include "pathfinder.h"
#include "graph.h"


//! @brief Результаты структурного анализа сети (пути, сечения, логические формы)
struct NetworkAnalysisResult {
    std::vector<Path> minimalPaths;
    std::vector<CutSet> minimalCuts;
    std::string dnf;
    std::string cnf;

    NetworkAnalysisResult() = default;
};

//! @brief Анализатор структурной связности сети (без вероятностных расчётов)
class NetworkAnalyzer {
public:
    explicit NetworkAnalyzer(const Graph &graph);

    //! @brief Проанализировать сеть между source и sink
    NetworkAnalysisResult analyze(unsigned int source, unsigned int sink);

    //! @brief Получить текстовое описание анализа
    std::string getAnalysisReport(const NetworkAnalysisResult &result) const;

private:
    const Graph &m_graph;
    PathFinder m_pathFinder;
};
