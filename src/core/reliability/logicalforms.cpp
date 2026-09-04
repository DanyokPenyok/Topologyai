#include "logicalforms.h"
#include <algorithm>
#include <sstream>
#include <set>

std::string LogicalForm::buildDNF(const std::vector<Path>& paths) {
    if (paths.empty()) return "0";
    std::ostringstream oss;
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i > 0) oss << " ∨ ";
        oss << "(";
        for (size_t j = 0; j < paths[i].edges.size(); ++j) {
            if (j > 0) oss << " ∧ ";
            oss << edgeToVariable(paths[i].edges[j].first, paths[i].edges[j].second);
        }
        oss << ")";
    }
    return oss.str();
}

std::string LogicalForm::buildCNF(const std::vector<CutSet>& cuts) {
    if (cuts.empty()) return "1";
    std::ostringstream oss;
    for (size_t i = 0; i < cuts.size(); ++i) {
        if (i > 0) oss << " ∧ ";
        oss << "(";
        for (size_t j = 0; j < cuts[i].edges.size(); ++j) {
            if (j > 0) oss << " ∨ ";
            oss << edgeToVariable(cuts[i].edges[j].first, cuts[i].edges[j].second);
        }
        oss << ")";
    }
    return oss.str();
}

std::string LogicalForm::buildProbabilisticDNF(const std::vector<Path>& paths,
                                              const std::map<std::pair<unsigned int, unsigned int>, double>& probabilities) {
    std::ostringstream oss;
    oss << "P(S) ≈ 1 - ∏(1 - P(path_i))\n";
    double reliability = 0.0;
    for (size_t i = 0; i < paths.size(); ++i) {
        double pathProb = 1.0;
        oss << "P(path_" << i+1 << ") = ";
        for (size_t j = 0; j < paths[i].edges.size(); ++j) {
            if (j > 0) oss << " * ";
            auto edge = paths[i].edges[j];
            double p = probabilities.count(edge) ? probabilities.at(edge) : 0.95;
            pathProb *= p;
            oss << p;
        }
        oss << " = " << pathProb << "\n";
        reliability = 1.0 - (1.0 - reliability) * (1.0 - pathProb);
    }
    oss << "Итоговая оценка надёжности: " << reliability;
    return oss.str();
}

std::vector<Path> LogicalForm::simplifyPaths(std::vector<Path> paths) {
    std::ranges::sort(paths, [](const Path& a, const Path& b) {
        return a.edges.size() < b.edges.size();
    });
    std::vector<Path> uniquePaths;
    for (const auto& path : paths) {
        bool isSubset = false;
        for (const auto& existing : uniquePaths) {
            if (std::ranges::includes(path.edges, existing.edges)) {
                isSubset = true; break;
            }
        }
        if (!isSubset) uniquePaths.push_back(path);
    }
    return uniquePaths;
}

std::string LogicalForm::edgeToVariable(unsigned int from, unsigned int to) {
    return "x" + std::to_string(from) + "_" + std::to_string(to);
}