#pragma once

#include "imaintenancestrategy.h"

//! @brief Стратегия 2.4: Профилактика без контроля, с отключением при проверке
//!
//! Периодические проверки требуют отключения системы.
//! Встроенный контроль не используется.
class StrategyOfflineCheck : public IMaintenanceStrategy {
public:
    StrategyOfflineCheck(double checkPeriod = 6.0, double checkWindow = 0.1);

    double detectFailure() override;
    double recover() override;
    bool checkMaintenance(double timeElapsed) override;
    const char *name() const override { return "2.4 Профилактика с отключением при проверке"; }

    void setCheckPeriod(double period);
    double getCheckPeriod() const;
    void setCheckWindow(double window);
    double getCheckWindow() const;

private:
    double m_checkPeriod;
    double m_checkWindow;

    static double uniformRandom(double min, double max);
};
