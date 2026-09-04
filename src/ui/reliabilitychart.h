#pragma once

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCharts>


class SimulationEngine;
//! @brief Виджет для отображения графика надёжности во времени
class ReliabilityChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReliabilityChartWidget(QWidget *parent = nullptr);

    //! @brief Обновить график данными из SimulationEngine
    void updateChart(const SimulationEngine *engine);

    //! @brief Очистить график
    void clear();

    //! @brief Экспорт данных в CSV
    void exportToCSV(const QString &fileName);

private:
    void setupUI();

    QChartView *m_chartView;
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QLabel *m_statusLabel;
    QPushButton *m_exportBtn;

    int m_pointCount = 0; //!< Счётчик точек для оптимизации (не рисовать каждую)
    static constexpr int SKIP_POINTS = 5; //!< Пропускать каждую N-ю точку для производительности
};
