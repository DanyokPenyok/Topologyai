#pragma once

#include "imaintenancestrategy.h"

//! @brief Стратегия 2.3: Профилактика без контроля, с мгновенным обнаружением
//!
//! При периодической проверке отказ обнаруживается мгновенно.
//! Система не отключается во время проверки работоспособности.
class StrategyInstantCheckDetection : public IMaintenanceStrategy {
public:
    StrategyInstantCheckDetection(double checkPeriod = 5.0, double checkWindow = 0.2);

    double detectFailure() override;
    double recover() override;
    bool checkMaintenance(double timeElapsed) override;
    const char *name() const override { return "2.3 Профилактика, мгновенное обнаружение при проверке"; }

    void setCheckPeriod(double period);
    double getCheckPeriod() const;
    void setCheckWindow(double window);
    double getCheckWindow() const;

private:
    double m_checkPeriod;
    double m_checkWindow;

    static double uniformRandom(double min, double max);
};
