#pragma once

#include "imaintenancestrategy.h"

//! @brief Стратегия 2.2: Профилактика без встроенного контроля, без отключения
//!
//! Профилактические проверки выполняются периодически, но система
//! не отключается во время проверки. Встроенный контроль не используется.
class StrategyNoControlCheck : public IMaintenanceStrategy {
public:
    StrategyNoControlCheck(double checkPeriod = 7.0, double checkWindow = 0.1);

    double detectFailure() override;
    double recover() override;
    bool checkMaintenance(double timeElapsed) override;
    const char *name() const override { return "2.2 Профилактика без контроля, без отключения"; }

    void setCheckPeriod(double period);
    double getCheckPeriod() const;
    void setCheckWindow(double window);
    double getCheckWindow() const;

private:
    double m_checkPeriod;
    double m_checkWindow;

    static double uniformRandom(double min, double max);
};
