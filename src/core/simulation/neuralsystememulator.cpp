#include "neuralsystememulator.h"
#include <algorithm>
#include <cmath>

NeuralSystemEmulator::NeuralSystemEmulator(Distribution dist) :
    m_distType(dist), m_gen(std::random_device{}()), m_uniform(0.0, 1.0) {}

NeuralSystemEmulator::NeuralSystemEmulator(const std::string &distMode) :
    m_gen(std::random_device{}()), m_uniform(0.0, 1.0) {
    setDistribution(distMode);
}

void NeuralSystemEmulator::setDistribution(Distribution dist) { m_distType = dist; }

void NeuralSystemEmulator::setDistribution(const std::string &distMode) {
    if (distMode == "exponential") {
        m_distType = Distribution::Exponential;
    } else if (distMode == "weibull") {
        m_distType = Distribution::Weibull;
    } else if (distMode == "normal") {
        m_distType = Distribution::Normal;
    } else {
        throw std::invalid_argument("Unknown dist_mode: " + distMode);
    }
}

double NeuralSystemEmulator::sampleFailureTime(NodeModel::DistributionType type, double expRate, double normMean,
                                               double normStd) {
    return generateFromDistribution((type == NodeModel::DistributionType::Exponential) ? Distribution::Exponential
                                                                                       : Distribution::Normal,
                                    expRate, 0, 0, normMean, normStd);
}

double NeuralSystemEmulator::sampleRecoveryTime(NodeModel::DistributionType type, double expRate, double normMean,
                                                double normStd) {
    return sampleFailureTime(type, expRate, normMean, normStd);
}

double NeuralSystemEmulator::sampleMaintenanceTime(NodeModel::DistributionType type, double expRate, double normMean,
                                                   double normStd) {
    return sampleRecoveryTime(type, expRate, normMean, normStd);
}

double NeuralSystemEmulator::generateFromDistribution(Distribution dist, double expRate, double weibullShape,
                                                      double weibullScale, double normMean, double normStd) {

    switch (dist) {
        case Distribution::Exponential: {
            std::exponential_distribution<double> d(expRate);
            return d(m_gen);
        }
        case Distribution::Weibull: {
            std::weibull_distribution<double> d(weibullShape, weibullScale);
            return d(m_gen);
        }
        case Distribution::Normal: {
            double u1, u2;

            //фикс -бесконечность
            do {
                u1 = m_uniform(m_gen);
            } while (u1 <= 0.0);

            u2 = m_uniform(m_gen);

            const double PI = 3.14159265358979323846;

            double z0 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * PI * u2);

            double result = normMean + z0 * normStd;

            return std::max(0.0, result);
        }
        default:
            throw std::runtime_error("Unknown distribution type");
    }
}

double NeuralSystemEmulator::sampleFailureTime() {
    return generateFromDistribution(m_distType, m_params.expFailureRate, m_params.weibullShape, m_params.weibullScale,
                                    m_params.normFailureMean, m_params.normFailureStd);
}

double NeuralSystemEmulator::sampleRecoveryTime() {
    return generateFromDistribution(m_distType, m_params.expRecoveryRate, m_params.weibullRecoveryShape,
                                    m_params.weibullRecoveryScale, m_params.normRecoveryMean, m_params.normRecoveryStd);
}

double NeuralSystemEmulator::sampleMaintenanceTime() {
    // Для профилактики используем те же параметры, что и для восстановления
    return sampleRecoveryTime();
}
