#include "reliabilitychart.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include "../core/simulation/simulationengine.h"

ReliabilityChartWidget::ReliabilityChartWidget(QWidget *parent) : QWidget(parent) { setupUI(); }

void ReliabilityChartWidget::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);

    auto *title = new QLabel("📈 График надёжности", this);
    title->setStyleSheet("font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(title);

    m_chartView = new QChartView(this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    auto *chart = new QChart();
    chart->setTitle("Коэффициент готовности (Kг)");
    chart->setAnimationOptions(QChart::NoAnimation);

    m_series = new QLineSeries();
    m_series->setName("Availability");
    m_series->setColor(QColor("#2ecc71"));
    m_series->setPen(QPen(QColor("#27ae60"), 2));


    m_axisX = new QValueAxis();
    m_axisX->setTitleText("Время (сек)");
    m_axisX->setLabelFormat("%.1f");

    m_axisY = new QValueAxis();
    m_axisY->setTitleText("Kг");
    m_axisY->setRange(0.0, 1.0);
    m_axisY->setLabelFormat("%.2f");

    chart->addSeries(m_series);
    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    m_axisX->setGridLineVisible(true);
    m_axisY->setGridLineVisible(true);
    m_axisX->setGridLineColor(QColor("#ecf0f1"));
    m_axisY->setGridLineColor(QColor("#ecf0f1"));

    m_chartView->setChart(chart);
    m_chartView->setMinimumHeight(300);
    mainLayout->addWidget(m_chartView);

    auto *footer = new QHBoxLayout();

    m_statusLabel = new QLabel("Ожидание данных...", this);
    m_statusLabel->setStyleSheet("color: #7f8c8d;");
    footer->addWidget(m_statusLabel);

    footer->addStretch();

    m_exportBtn = new QPushButton("💾 Экспорт CSV", this);
    m_exportBtn->setFixedWidth(120);
    connect(m_exportBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Экспорт данных графика", "reliability_data.csv",
                                                        "CSV Files (*.csv)");
        if (!fileName.isEmpty()) {
            exportToCSV(fileName);
        }
    });
    footer->addWidget(m_exportBtn);

    mainLayout->addLayout(footer);
    clear();
}

void ReliabilityChartWidget::updateChart(const SimulationEngine *engine) {
    if (!engine)
        return;

    const auto &history = engine->getReliabilityHistory();
    if (history.empty()) {
        m_statusLabel->setText("Нет данных для отображения");
        return;
    }

    if (m_pointCount > 0 && history.size() - m_pointCount < SKIP_POINTS) {
        return;
    }

    m_series->clear();

    for (size_t i = 0; i < history.size(); ++i) {
        if (i % SKIP_POINTS == 0) {
            m_series->append(history[i].timestamp, history[i].availability);
        }
    }

    if (!history.empty()) {
        m_axisX->setRange(0, history.back().timestamp);

        double minVal = 1.0;
        double maxVal = 0.0;
        for (const auto &[timestamp, availability]: history) {
            if (availability < minVal)
                minVal = availability;
            if (availability > maxVal)
                maxVal = availability;
        }

        const double range = maxVal - minVal;
        const double padding = (range > 0.0) ? range * 0.1 : 0.1;

        double newYMin = std::max(0.0, minVal - padding);
        double newYMax = std::min(1.0, maxVal + padding);

        if (newYMax - newYMin < 0.2) {
            const double center = (newYMin + newYMax) / 2.0;
            newYMin = std::max(0.0, center - 0.1);
            newYMax = std::min(1.0, center + 0.1);
        }

        m_axisY->setRange(newYMin, newYMax);
    }

    m_pointCount = static_cast<int>(history.size());
    m_statusLabel->setText(QString("Записано точек: %1 | Текущий Kг: %2")
                                   .arg(history.size())
                                   .arg(engine->getGlobalAvailability(), 0, 'f', 4));

    m_chartView->chart()->update();
}

void ReliabilityChartWidget::clear() {
    m_series->clear();
    m_axisX->setRange(0, 10);
    m_axisY->setRange(0.0, 1.0);
    m_statusLabel->setText("Ожидание данных...");
    m_pointCount = 0;
    m_chartView->chart()->update();
}

void ReliabilityChartWidget::exportToCSV(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл");
        return;
    }

    QTextStream out(&file);
    out << "timestamp,availability\n";

    for (int i = 0; i < m_series->count(); ++i) {
        QPointF point = m_series->at(i);
        out << point.x() << "," << point.y() << "\n";
    }

    file.close();
    QMessageBox::information(this, "Успех", "Данные экспортированы: " + fileName);
}
