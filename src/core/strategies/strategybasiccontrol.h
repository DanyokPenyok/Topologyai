#pragma once

#include <random>
#include "imaintenancestrategy.h"

//! @brief Стратегия 1.1: Встроенный контроль без обнаружения места отказа
//!
//! Базовая стратегия с эмуляцией времени обнаружения и восстановления
//! по равномерному распределению. Профилактика не предусмотрена.
class StrategyBasicControl : public IMaintenanceStrategy {
public:
    double detectFailure() override;
    double recover() override;
    bool checkMaintenance(double timeElapsed) override;
    const char *name() const override { return "1.1 Контроль без локализации отказа"; }

private:
    static double uniformRandom(double min, double max);
};
