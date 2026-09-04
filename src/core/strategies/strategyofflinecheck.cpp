#include "strategyofflinecheck.h"
#include <cmath>
#include <random>

StrategyOfflineCheck::StrategyOfflineCheck(double checkPeriod, double checkWindow) :
    m_checkPeriod(checkPeriod), m_checkWindow(checkWindow) {}

double StrategyOfflineCheck::uniformRandom(double min, double max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

double StrategyOfflineCheck::detectFailure() { return uniformRandom(1.0, 2.5); }

double StrategyOfflineCheck::recover() { return uniformRandom(1.0, 2.5); }

bool StrategyOfflineCheck::checkMaintenance(double timeElapsed) {
    return std::fmod(timeElapsed, m_checkPeriod) < m_checkWindow;
}

void StrategyOfflineCheck::setCheckPeriod(double period) { m_checkPeriod = period; }
double StrategyOfflineCheck::getCheckPeriod() const { return m_checkPeriod; }
void StrategyOfflineCheck::setCheckWindow(double window) { m_checkWindow = window; }
double StrategyOfflineCheck::getCheckWindow() const { return m_checkWindow; }
