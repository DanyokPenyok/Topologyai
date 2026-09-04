#include "strategyfixedintervalcontrol.h"
#include <random>

StrategyFixedIntervalControl::StrategyFixedIntervalControl(double interval) :
    m_maintenanceInterval(interval), m_lastMaintenance(0.0) {}

double StrategyFixedIntervalControl::uniformRandom(double min, double max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

double StrategyFixedIntervalControl::detectFailure() { return uniformRandom(0.5, 1.0); }

double StrategyFixedIntervalControl::recover() { return uniformRandom(1.0, 2.0); }

bool StrategyFixedIntervalControl::checkMaintenance(double timeElapsed) {
    if (timeElapsed - m_lastMaintenance >= m_maintenanceInterval) {
        m_lastMaintenance = timeElapsed;
        return true;
    }
    return false;
}

void StrategyFixedIntervalControl::setMaintenanceInterval(double interval) { m_maintenanceInterval = interval; }

double StrategyFixedIntervalControl::getMaintenanceInterval() const { return m_maintenanceInterval; }
