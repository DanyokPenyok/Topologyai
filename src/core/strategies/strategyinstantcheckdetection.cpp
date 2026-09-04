#include "strategyinstantcheckdetection.h"
#include <cmath>
#include <random>

StrategyInstantCheckDetection::StrategyInstantCheckDetection(double checkPeriod, double checkWindow) :
    m_checkPeriod(checkPeriod), m_checkWindow(checkWindow) {}

double StrategyInstantCheckDetection::uniformRandom(double min, double max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

double StrategyInstantCheckDetection::detectFailure() {
    return 0.0; // Мгновенное обнаружение при проверке
}

double StrategyInstantCheckDetection::recover() { return uniformRandom(1.0, 2.0); }

bool StrategyInstantCheckDetection::checkMaintenance(double timeElapsed) {
    return std::fmod(timeElapsed, m_checkPeriod) < m_checkWindow;
}

void StrategyInstantCheckDetection::setCheckPeriod(double period) { m_checkPeriod = period; }
double StrategyInstantCheckDetection::getCheckPeriod() const { return m_checkPeriod; }
void StrategyInstantCheckDetection::setCheckWindow(double window) { m_checkWindow = window; }
double StrategyInstantCheckDetection::getCheckWindow() const { return m_checkWindow; }
