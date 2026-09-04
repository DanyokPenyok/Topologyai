#pragma once

#include "imaintenancestrategy.h"

//! @brief Стратегия 3: Встроенный контроль + фиксированный период профилактики
//!
//! Комбинирует непрерывный встроенный контроль с плановой
//! профилактикой по фиксированному интервалу времени.
class StrategyFixedIntervalControl : public IMaintenanceStrategy {
public:
    StrategyFixedIntervalControl(double interval = 8.0);

    double detectFailure() override;
    double recover() override;
    bool checkMaintenance(double timeElapsed) override;
    const char *name() const override { return "3. Встроенный контроль + фиксированная профилактика"; }

    void setMaintenanceInterval(double interval);
    double getMaintenanceInterval() const;

private:
    double m_maintenanceInterval;
    double m_lastMaintenance;

    static double uniformRandom(double min, double max);
};
