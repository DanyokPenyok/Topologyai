#include "strategynocontrolcheck.h"
#include <cmath>
#include <random>

StrategyNoControlCheck::StrategyNoControlCheck(double checkPeriod, double checkWindow) :
    m_checkPeriod(checkPeriod), m_checkWindow(checkWindow) {}

double StrategyNoControlCheck::uniformRandom(double min, double max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

double StrategyNoControlCheck::detectFailure() { return uniformRandom(2.0, 4.0); }

double StrategyNoControlCheck::recover() { return uniformRandom(2.0, 4.0); }

bool StrategyNoControlCheck::checkMaintenance(double timeElapsed) {
    // Проверка в окне времени вокруг кратных периоду
    return std::fmod(timeElapsed, m_checkPeriod) < m_checkWindow;
}

void StrategyNoControlCheck::setCheckPeriod(double period) { m_checkPeriod = period; }
double StrategyNoControlCheck::getCheckPeriod() const { return m_checkPeriod; }
void StrategyNoControlCheck::setCheckWindow(double window) { m_checkWindow = window; }
double StrategyNoControlCheck::getCheckWindow() const { return m_checkWindow; }
