#pragma once

//! @brief Интерфейс стратегии технического обслуживания
//! Определяет контракт для всех стратегий обслуживания системы.
class IMaintenanceStrategy {
public:
    virtual ~IMaintenanceStrategy() = default;

    //! @return Время обнаружения отказа
    virtual double detectFailure() = 0;

    //! @return Время восстановления
    virtual double recover() = 0;

    //! @param timeElapsed Общее время работы
    //! @return true, если требуется профилактика
    virtual bool checkMaintenance(double timeElapsed) = 0;

    //! @return Название стратегии для отчётов
    virtual const char *name() const = 0;
};
