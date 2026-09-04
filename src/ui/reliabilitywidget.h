#pragma once

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "../core/reliability/networkanalyzer.h"
#include "graph.h"

//! @brief Виджет для отображения структурного анализа сети (ДНФ/КНФ)
class ReliabilityWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReliabilityWidget(QWidget *parent = nullptr);
    void setGraph(Graph *graph);
    void analyzeNetwork(unsigned int source, unsigned int sink);

private slots:
    void onAnalyzeClicked();
    void onExportClicked();
    void onSaveResultsClicked();

private:
    void setupUI();
    void displayResults(const NetworkAnalysisResult &result);

    Graph *m_graph = nullptr;
    std::unique_ptr<NetworkAnalyzer> m_analyzer;

    QSpinBox *m_sourceSpin;
    QSpinBox *m_sinkSpin;
    QPushButton *m_analyzeBtn;
    QPushButton *m_exportBtn;

    QTextEdit *m_pathsEdit;
    QTextEdit *m_dnfEdit;
    QTextEdit *m_cutsEdit;
    QTextEdit *m_cnfEdit;
    QLabel *m_infoLabel;
};
