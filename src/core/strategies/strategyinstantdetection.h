#pragma once

#include "imaintenancestrategy.h"

//! @brief Стратегия 1.2: Встроенный контроль с мгновенным обнаружением отказа
//!
//! Отказ обнаруживается мгновенно, время восстановления моделируется
//! равномерным распределением. Профилактика не предусмотрена.
class StrategyInstantDetection : public IMaintenanceStrategy {
public:
    double detectFailure() override;
    double recover() override;
    bool checkMaintenance(double timeElapsed) override;
    const char *name() const override { return "1.2 Мгновенное обнаружение отказа"; }

private:
    static double uniformRandom(double min, double max);
};
