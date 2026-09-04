#pragma once

#include "imaintenancestrategy.h"

//! @brief Стратегия 4: Периодический контроль + аварийная профилактика
//!
//! Периодические проверки системы. При обнаружении отказа
//! выполняются аварийные профилактические работы.
class StrategyPeriodicControlEmergencyRecovery : public IMaintenanceStrategy {
public:
    StrategyPeriodicControlEmergencyRecovery(double controlInterval = 6.0);

    double detectFailure() override;
    double recover() override;
    bool checkMaintenance(double timeElapsed) override;
    const char *name() const override { return "4. Периодический контроль + аварийная профилактика"; }

    void setControlInterval(double interval);
    double getControlInterval() const;

private:
    double m_controlInterval;
    double m_lastCheck;

    static double uniformRandom(double min, double max);
};
