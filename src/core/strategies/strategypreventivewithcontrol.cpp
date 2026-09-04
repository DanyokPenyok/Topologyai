#include "strategypreventivewithcontrol.h"
#include <random>

StrategyPreventiveWithControl::StrategyPreventiveWithControl(double interval) :
    m_lastMaintenance(0.0), m_maintenanceInterval(interval) {}

double StrategyPreventiveWithControl::uniformRandom(double min, double max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

double StrategyPreventiveWithControl::detectFailure() { return uniformRandom(0.5, 1.5); }

double StrategyPreventiveWithControl::recover() { return uniformRandom(1.5, 3.0); }

bool StrategyPreventiveWithControl::checkMaintenance(double timeElapsed) {
    if (timeElapsed - m_lastMaintenance >= m_maintenanceInterval) {
        m_lastMaintenance = timeElapsed;
        return true;
    }
    return false;
}

void StrategyPreventiveWithControl::setMaintenanceInterval(double interval) { m_maintenanceInterval = interval; }

double StrategyPreventiveWithControl::getMaintenanceInterval() const { return m_maintenanceInterval; }
