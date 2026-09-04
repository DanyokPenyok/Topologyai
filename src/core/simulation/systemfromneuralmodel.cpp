#include "systemfromneuralmodel.h"

SystemFromNeuralModel::SystemFromNeuralModel(
    IMaintenanceStrategy* strategy, 
    const NeuralSystemEmulator& emulator)
    : m_strategy(strategy)
    , m_emulator(emulator)
    , m_timeElapsed(0.0)
    , m_totalDowntime(0.0)
    , m_totalRuntime(0.0)
    , m_cycles(0) {}

void SystemFromNeuralModel::runCycle() {
    double nextFailureTime = m_emulator.sampleFailureTime();
    double workingTime = nextFailureTime;
    
    if (m_strategy->checkMaintenance(m_timeElapsed + workingTime)) {
        double maintenanceTime = m_emulator.sampleMaintenanceTime();
        m_totalDowntime += maintenanceTime;
        m_timeElapsed += maintenanceTime;
        m_totalRuntime += workingTime;
    } else {
        double detectionTime = m_strategy->detectFailure();
        double recoveryTime = m_strategy->recover();
        
        m_totalDowntime += detectionTime + recoveryTime;
        m_timeElapsed += workingTime + detectionTime + recoveryTime;
        m_totalRuntime += workingTime;
    }
    
    ++m_cycles;
}

double SystemFromNeuralModel::getAvailability() const {
    double total = m_totalRuntime + m_totalDowntime;
    return (total > 0.0) ? m_totalRuntime / total : 0.0;
}