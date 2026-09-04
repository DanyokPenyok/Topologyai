#pragma once

#include <sstream>
#include <string>
#include "pathfinder.h"

//! @brief Представление логической функции надежности
class LogicalForm {
public:
    //! @brief Построить ДНФ из набора путей
    static std::string buildDNF(const std::vector<Path> &paths);

    //! @brief Построить КНФ из набора сечений
    static std::string buildCNF(const std::vector<CutSet> &cuts);

    //! @brief Построить ДНФ с вероятностями (для расчёта надёжности)
    static std::string
    buildProbabilisticDNF(const std::vector<Path> &paths,
                          const std::map<std::pair<unsigned int, unsigned int>, double> &probabilities);

    //! @brief Упростить логическое выражение (удалить поглощаемые термы)
    static std::vector<Path> simplifyPaths(std::vector<Path> paths);

private:
    static std::string edgeToVariable(unsigned int from, unsigned int to);
};
