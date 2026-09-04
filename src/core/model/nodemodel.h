#pragma once
#include <memory>
#include <string>
#include <vector>
#include "imaintenancestrategy.h"

//! @brief Чистая модель вершины графа
class NodeModel {
public:
    //! @brief Состояние вершины
    enum class State { Operational, Failed, Maintenance };

    //! @brief Тип распределения времени событий
    enum class DistributionType { Exponential, Normal };

    //! @brief Параметры распределения для узла
    struct DistributionParams {
        DistributionType type = DistributionType::Exponential;
        double expRate = 0.1;
        double normMean = 10.0;
        double normStd = 2.0;
    };

    //! @brief Получить параметры распределения
    const DistributionParams &getDistributionParams() const;

    //! @brief Установить параметры распределения
    void setDistributionParams(const DistributionParams &params);
    void setDistributionType(DistributionType type);
    void setExpRate(double rate);
    void setNormalParams(double mean, double std);

    explicit NodeModel(unsigned int id);

    unsigned int id() const;
    State state() const;
    void setState(State s);
    double reliability() const;
    void setReliability(double r);

    //! @brief Привязка стратегии обслуживания
    void setStrategy(std::unique_ptr<IMaintenanceStrategy> strategy);
    IMaintenanceStrategy *strategy() const;

    //! @brief Очередь входящих сообщений
    void addMessage(const std::string &msg);
    const std::vector<std::string> &inbox() const;
    void clearInbox();

    double getNextFailureTime() const { return m_nextFailureTime; }
    void setNextFailureTime(double t) { m_nextFailureTime = t; }

private:
    unsigned int m_id;
    State m_state;
    double m_reliability;
    std::unique_ptr<IMaintenanceStrategy> m_strategy;
    std::vector<std::string> m_inbox;

    DistributionParams m_distParams;
    double m_nextFailureTime = 0.0;
};
