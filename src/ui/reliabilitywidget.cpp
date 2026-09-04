#include "reliabilitywidget.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include "../core/reliability/networkanalyzer.h"
#include "graph.h"
#include "mainwindow.h"

ReliabilityWidget::ReliabilityWidget(QWidget *parent) : QWidget(parent) { setupUI(); }

void ReliabilityWidget::setGraph(Graph *graph) {
    m_graph = graph;
    if (m_graph) {
        if (const int max = static_cast<int>(m_graph->getAmount()) - 1; max > 0) {
            m_sourceSpin->setMaximum(max);
            m_sinkSpin->setMaximum(max);
            m_sinkSpin->setValue(max);
        }
        m_analyzer = std::make_unique<NetworkAnalyzer>(*m_graph);
    }
}

void ReliabilityWidget::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    auto *controlGroup = new QGroupBox("Анализ связности", this);
    auto *controlLayout = new QHBoxLayout(controlGroup);
    controlLayout->setContentsMargins(6, 12, 6, 6);
    controlLayout->setSpacing(8);

    controlLayout->addWidget(new QLabel("Источник:"));
    m_sourceSpin = new QSpinBox(this);
    m_sourceSpin->setRange(0, 100);
    m_sourceSpin->setFixedWidth(60);
    controlLayout->addWidget(m_sourceSpin);

    controlLayout->addWidget(new QLabel("Сток:"));
    m_sinkSpin = new QSpinBox(this);
    m_sinkSpin->setRange(0, 100);
    m_sinkSpin->setFixedWidth(60);
    controlLayout->addWidget(m_sinkSpin);

    m_analyzeBtn = new QPushButton("Анализ", this);
    m_analyzeBtn->setFixedHeight(30);
    m_analyzeBtn->setToolTip("Анализировать");
    controlLayout->addWidget(m_analyzeBtn);

    m_exportBtn = new QPushButton("Экспорт", this);
    m_exportBtn->setFixedHeight(30);
    m_exportBtn->setToolTip("Экспорт");
    controlLayout->addWidget(m_exportBtn);

    auto *footer = new QHBoxLayout();
    footer->setContentsMargins(0, 4, 0, 0);

    auto *saveResultsBtn = new QPushButton("💾 Сохранить итоги", this);
    saveResultsBtn->setToolTip("Сохранить статистику по узлам в CSV");
    connect(saveResultsBtn, &QPushButton::clicked, this, &ReliabilityWidget::onSaveResultsClicked);
    footer->addWidget(saveResultsBtn);

    footer->addStretch();
    mainLayout->addLayout(footer); // <-- Добавляем footer в основной лейаут

    connect(m_analyzeBtn, &QPushButton::clicked, this, &ReliabilityWidget::onAnalyzeClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &ReliabilityWidget::onExportClicked);


    controlLayout->addStretch();
    mainLayout->addWidget(controlGroup);

    const auto resultsLayout = new QHBoxLayout;
    resultsLayout->setSpacing(8);

    const auto leftCol = new QVBoxLayout;
    leftCol->setSpacing(4);

    const auto pathsLabel = new QLabel("<b>Мин. пути:</b>");
    pathsLabel->setStyleSheet("font-weight: bold;");
    m_pathsEdit = new QTextEdit(this);
    m_pathsEdit->setReadOnly(true);
    m_pathsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const auto dnfLabel = new QLabel("<b>ДНФ:</b>");
    dnfLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_dnfEdit = new QTextEdit(this);
    m_dnfEdit->setReadOnly(true);
    m_dnfEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    leftCol->addWidget(pathsLabel);
    leftCol->addWidget(m_pathsEdit, 1);
    leftCol->addWidget(dnfLabel);
    leftCol->addWidget(m_dnfEdit, 1);
    resultsLayout->addLayout(leftCol, 1);

    const auto rightCol = new QVBoxLayout;
    rightCol->setSpacing(4);

    const auto cutsLabel = new QLabel("<b>Мин. сечения:</b>");
    cutsLabel->setStyleSheet("font-weight: bold;");
    m_cutsEdit = new QTextEdit(this);
    m_cutsEdit->setReadOnly(true);
    m_cutsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const auto cnfLabel = new QLabel("<b>КНФ:</b>");
    cnfLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_cnfEdit = new QTextEdit(this);
    m_cnfEdit->setReadOnly(true);
    m_cnfEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    rightCol->addWidget(cutsLabel);
    rightCol->addWidget(m_cutsEdit, 1);
    rightCol->addWidget(cnfLabel);
    rightCol->addWidget(m_cnfEdit, 1);
    resultsLayout->addLayout(rightCol, 1);

    mainLayout->addLayout(resultsLayout, 1);

    m_infoLabel = new QLabel("Статус: ожидание анализа", this);
    m_infoLabel->setStyleSheet("font-size: 12px; color: #7f8c8d;");
    m_infoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_infoLabel);

    connect(m_analyzeBtn, &QPushButton::clicked, this, &ReliabilityWidget::onAnalyzeClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &ReliabilityWidget::onExportClicked);
}

void ReliabilityWidget::onAnalyzeClicked() {
    if (!m_graph || m_graph->getAmount() < 2) {
        QMessageBox::warning(this, "Ошибка", "Граф должен содержать минимум 2 узла.");
        return;
    }
    const unsigned int src = static_cast<unsigned int>(m_sourceSpin->value());
    const unsigned int snk = static_cast<unsigned int>(m_sinkSpin->value());
    if (src == snk) {
        QMessageBox::warning(this, "Ошибка", "Источник и сток не могут совпадать.");
        return;
    }
    analyzeNetwork(src, snk);
}

void ReliabilityWidget::analyzeNetwork(unsigned int source, unsigned int sink) {
    m_analyzer = std::make_unique<NetworkAnalyzer>(*m_graph);
    auto result = m_analyzer->analyze(source, sink);
    displayResults(result);
    m_infoLabel->setText(QString("Анализ завершён: %1 путей, %2 сечений")
                                 .arg(result.minimalPaths.size())
                                 .arg(result.minimalCuts.size()));
}

void ReliabilityWidget::displayResults(const NetworkAnalysisResult &result) {
    m_pathsEdit->clear();
    for (const auto &p: result.minimalPaths)
        m_pathsEdit->append(QString::fromStdString(p.toString()));

    m_dnfEdit->setText(QString::fromStdString(result.dnf));

    m_cutsEdit->clear();
    for (const auto &c: result.minimalCuts)
        m_cutsEdit->append(QString::fromStdString(c.toString()));

    m_cnfEdit->setText(QString::fromStdString(result.cnf));
}

void ReliabilityWidget::onExportClicked() {
    const QString fileName = QFileDialog::getSaveFileName(this, "Экспорт результатов", "", "Text Files (*.txt)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл.");
        return;
    }

    QTextStream out(&file);
    out << "=== СТРУКТУРНЫЙ АНАЛИЗ СЕТИ ===\n";
    out << "ПУТИ:\n" << m_pathsEdit->toPlainText() << "\n\n";
    out << "ДНФ:\n" << m_dnfEdit->toPlainText() << "\n\n";
    out << "СЕЧЕНИЯ:\n" << m_cutsEdit->toPlainText() << "\n\n";
    out << "КНФ:\n" << m_cnfEdit->toPlainText() << "\n";

    file.close();
    QMessageBox::information(this, "Успех", "Отчёт сохранён: " + fileName);
}

void ReliabilityWidget::onSaveResultsClicked() {
    auto *mainWindow = qobject_cast<MainWindow *>(window());
    if (!mainWindow) {
        QMessageBox::warning(this, "Ошибка", "Не удалось получить доступ к основному окну.");
        return;
    }

    if (!mainWindow->m_simulation || mainWindow->m_simulation->isRunning()) {
        QMessageBox::warning(this, "Предупреждение",
                             "Симуляция должна быть остановлена перед сохранением результатов.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить результаты моделирования",
                                                    "simulation_results.csv", "CSV Files (*.csv)");

    if (fileName.isEmpty())
        return;

    if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
        fileName += ".csv";
    }

    if (mainWindow->saveSimulationResults(fileName)) {
        QMessageBox::information(this, "Успех",
                                 "Результаты успешно сохранены:\n" + fileName +
                                         "\n\nФормат:\n"
                                         "• NodeID — идентификатор узла\n"
                                         "• Availability(%) — коэффициент готовности в процентах\n"
                                         "• Runtime(s) — общее время работы (сек)\n"
                                         "• Downtime(s) — общее время простоя (сек)\n"
                                         "• Failures — количество отказов\n"
                                         "• Maintenances — количество профилактик");
    } else {
        QMessageBox::critical(this, "Ошибка",
                              "Не удалось сохранить файл.\n\nВозможные причины:\n"
                              "• Нет прав на запись в папку\n"
                              "• Файл открыт в другой программе\n"
                              "• Недостаточно места на диске");
    }
}
