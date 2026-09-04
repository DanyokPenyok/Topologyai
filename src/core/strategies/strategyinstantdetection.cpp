#include "strategyinstantdetection.h"
#include <random>

double StrategyInstantDetection::uniformRandom(double min, double max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

double StrategyInstantDetection::detectFailure() {
    return 0.0; // Мгновенное обнаружение
}

double StrategyInstantDetection::recover() { return uniformRandom(2.0, 4.0); }

bool StrategyInstantDetection::checkMaintenance(double) {
    return false; // Профилактика не предусмотрена
}
