#include "strategyperiodiccontrolemergencyrecovery.h"
#include <random>

StrategyPeriodicControlEmergencyRecovery::StrategyPeriodicControlEmergencyRecovery(double controlInterval) :
    m_controlInterval(controlInterval), m_lastCheck(0.0) {}

double StrategyPeriodicControlEmergencyRecovery::uniformRandom(double min, double max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

double StrategyPeriodicControlEmergencyRecovery::detectFailure() { return uniformRandom(0.0, 0.5); }

double StrategyPeriodicControlEmergencyRecovery::recover() { return uniformRandom(2.0, 5.0); }

bool StrategyPeriodicControlEmergencyRecovery::checkMaintenance(double timeElapsed) {
    if (timeElapsed - m_lastCheck >= m_controlInterval) {
        m_lastCheck = timeElapsed;
        return true;
    }
    return false;
}

void StrategyPeriodicControlEmergencyRecovery::setControlInterval(double interval) { m_controlInterval = interval; }

double StrategyPeriodicControlEmergencyRecovery::getControlInterval() const { return m_controlInterval; }
