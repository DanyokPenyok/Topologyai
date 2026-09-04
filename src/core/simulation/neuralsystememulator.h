#pragma once

#include <nodemodel.h>
#include <random>
#include <stdexcept>
#include <string>

//!
//!   @brief Эмулятор временных потоков для имитационного моделирования
//!
//!   Генерирует случайные величины для времени отказов, восстановлений
//!   и профилактических работ по заданному закону распределения.
//!

class NeuralSystemEmulator {
public:
    enum class Distribution { Exponential, Weibull, Normal };

    NeuralSystemEmulator(Distribution dist = Distribution::Exponential);
    NeuralSystemEmulator(const std::string &distMode);

    double sampleFailureTime();
    double sampleRecoveryTime();
    double sampleMaintenanceTime();

    void setDistribution(Distribution dist);
    void setDistribution(const std::string &distMode);

    double sampleFailureTime(NodeModel::DistributionType type, double expRate, double normMean, double normStd);
    double sampleRecoveryTime(NodeModel::DistributionType type, double expRate, double normMean, double normStd);
    double sampleMaintenanceTime(NodeModel::DistributionType type, double expRate, double normMean, double normStd);

private:
    Distribution m_distType;
    std::mt19937 m_gen;
    std::uniform_real_distribution<double> m_uniform;

    // Параметры распределений
    struct DistParams {
        // Exponential
        double expFailureRate = 0.1;
        double expRecoveryRate = 0.2;
        double expMaintenanceRate = 0.2;

        // Weibull
        double weibullShape = 1.5;
        double weibullScale = 10.0;
        double weibullRecoveryShape = 2.0;
        double weibullRecoveryScale = 5.0;

        // Normal
        double normFailureMean = 10.0;
        double normFailureStd = 3.0;
        double normRecoveryMean = 5.0;
        double normRecoveryStd = 1.5;
    };

    DistParams m_params;

    double generateFromDistribution(Distribution dist, double expRate, double weibullShape, double weibullScale,
                                    double normMean, double normStd);
};
