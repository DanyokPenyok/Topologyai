#pragma once

#include "imaintenancestrategy.h"
#include "neuralsystememulator.h"

//! @brief Модель системы для имитационного эксперимента
//!
//! Запускает циклы работы системы с выбранной стратегией обслуживания,
//! накапливает статистику для расчёта коэффициента готовности.
class SystemFromNeuralModel {
public:
    SystemFromNeuralModel(IMaintenanceStrategy* strategy,
                         const NeuralSystemEmulator& emulator);

    void runCycle();
    double getAvailability() const;

    double getTotalRuntime() const { return m_totalRuntime; }
    double getTotalDowntime() const { return m_totalDowntime; }
    int getCycles() const { return m_cycles; }

private:
    IMaintenanceStrategy* m_strategy;
    NeuralSystemEmulator m_emulator;

    double m_timeElapsed;
    double m_totalDowntime;
    double m_totalRuntime;
    int m_cycles;
};