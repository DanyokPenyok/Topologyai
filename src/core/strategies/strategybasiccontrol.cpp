#include "strategybasiccontrol.h"

double StrategyBasicControl::uniformRandom(double min, double max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

double StrategyBasicControl::detectFailure() { return uniformRandom(2.0, 4.0); }

double StrategyBasicControl::recover() { return uniformRandom(3.0, 6.0); }

bool StrategyBasicControl::checkMaintenance(double) { return false; }
