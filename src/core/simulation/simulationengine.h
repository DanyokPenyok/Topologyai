#pragma once

#include <QString>
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include "messagebus.h"
#include "neuralsystememulator.h"
#include "nodemodel.h"


struct ReliabilityPoint {
    double timestamp;
    double availability;
};

//! @brief Двигатель имитационного моделирования надёжности сети
//!
//! Выполняет пошаговую симуляцию работы графа, где каждая вершина
//! имеет стратегию обслуживания. Учитывает отказы, восстановления
//! и профилактические работы для расчёта коэффициента готовности.
class SimulationEngine {
public:
    //! @brief Колбэк для уведомления внешнего кода о событиях
    using StepCallback = std::function<void()>;
    using EventCallback = std::function<void(unsigned int nodeId, const std::string &event)>;

    //! @brief Конструктор
    //! @param dt Шаг симуляции (единицы модельного времени)
    //! @param distMode Режим распределения ("exponential", "weibull", "normal")
    explicit SimulationEngine(double dt = 0.1, const std::string &distMode = "exponential");

    //! @brief Привязка набора вершин для симуляции
    void setNodes(const std::vector<std::shared_ptr<NodeModel>> &nodes);

    //! @brief Выполнить один такт симуляции
    void step();

    //! @brief Запустить/остановить симуляцию
    void start();
    void stop();
    bool isRunning() const;

    //! @brief Сброс статистики и времени
    void reset();

    //! @brief Геттеры состояния
    double currentTime() const;
    double getGlobalAvailability() const;
    double getNodeAvailability(unsigned int nodeId) const;

    //! @brief Установка колбэков
    void setStepCallback(StepCallback cb);
    void setEventCallback(EventCallback cb);

    //! @brief Доступ к шине сообщений (для отладки/расширения)
    MessageBus &messageBus();

    //! @brief Включить/выключить запись истории надёжности
    void setRecordHistory(bool enabled) { m_recordHistory = enabled; }

    // Тихий режим
    void setSilent(bool silent) { m_silent = silent; }
    bool isSilent() const { return m_silent; }

    //! @brief Получить историю надёжности для построения графика
    const std::vector<ReliabilityPoint> &getReliabilityHistory() const { return m_reliabilityHistory; }

    //! @brief Очистить историю (перед новым запуском)
    void clearReliabilityHistory() { m_reliabilityHistory.clear(); }

    //! @brief Структура итоговой статистики по узлу
    struct NodeFinalStats {
        unsigned int id; //!< ID узла
        double availabilityPercent; //!< Коэффициент готовности в %
        double totalRuntime; //!< Общее время работы
        double totalDowntime; //!< Общее время простоя
        unsigned int failureCount; //!< Количество отказов
        unsigned int maintenanceCount; //!< Количество профилактик
    };

    //! @brief Получить итоговую статистику по всем узлам
    std::vector<NodeFinalStats> getFinalStats() const;

    //! @brief Экспорт статистики в CSV
    bool exportStatsToCSV(const QString &fileName) const;

private:
    //! @brief Обработка состояния одной вершины на такте
    void processNode(NodeModel &node);

    //! @brief Генерация события отказа для вершины
    void triggerFailure(NodeModel &node);

    //! @brief Обработка восстановления вершины
    void handleRecovery(NodeModel &node);

    double m_dt;
    double m_currentTime;
    bool m_running;

    std::vector<std::shared_ptr<NodeModel>> m_nodes;
    MessageBus m_bus;
    NeuralSystemEmulator m_emulator;

    // Статистика глобальная
    double m_totalRuntime;
    double m_totalDowntime;

    // Статистика по вершинам
    struct NodeStats {
        double runtime = 0.0;
        double downtime = 0.0;
        int failureCount = 0;
        int maintenanceCount = 0;
    };
    std::map<unsigned int, NodeStats> m_nodeStats;
    std::map<unsigned int, double> m_maintenanceEndTimes;

    StepCallback m_stepCallback;
    EventCallback m_eventCallback;

    std::vector<ReliabilityPoint> m_reliabilityHistory;
    bool m_recordHistory = false;
    bool m_silent = false;
};
