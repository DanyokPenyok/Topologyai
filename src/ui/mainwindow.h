#pragma once

#include <QDockWidget>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include "graph.h"
#include "qactiongroup.h"
#include "qpushbutton.h"
#include "qshortcut.h"
#include "qspinbox.h"
#include "qtableview.h"
#include "reliabilitychart.h"
#include "reliabilitywidget.h"
#include "simulationengine.h"

// Форвард-декларация
class NodeInfoWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

struct GraphState {
    unsigned int amount;
    Matrix2D adj;
    Matrix2D flow;
    Matrix2D band;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    Graph graph;
    ~MainWindow();

    //! @brief Сохранить результаты моделирования в файл
    bool saveSimulationResults(const QString &fileName);
    SimulationEngine *getSimulationEngine() const { return m_simulation.get(); }
    std::unique_ptr<SimulationEngine> m_simulation;

private slots:
    void buttonClearConsoleClicked();
    void viewModeChecked(bool checked);
    void myCopy();
    void myPaste();
    void setNodesAmountMatrix(QTableView *table, int newAmount);
    void saveToFile();
    void onSimulationStart();
    void onSimulationStop();
    void onSimulationStopped();
    void onGenerateAIDataset();
    void onExportCurrentGraphAI();
    void onOptimizeGraphAI();
    void onImportAIRecommendation();

private:
    void applyRecommendationJson(const QString &filePath);
    Ui::MainWindow *ui;
    QMap<QWidget *, QPushButton *> pins;
    QMap<QString, QWidget *> docksViewMode;

    QList<GraphState> undoStack;
    QList<GraphState> redoStack;
    GraphState captureState();
    void saveState();
    void restoreState(const GraphState &state);
    void setUndoRedoEnabled(bool enabled);

    void unpinTab(int index);
    void pinTab();
    void pasteClipboardToTable(QTableView *dest);
    void copyTableToClipboard(QTableView *src);
    void applyGraphMatrix(QTableView *table);
    void applyEdgesList(QTableView *table);
    void updateEdgesList(QTableView *list);
    void updateTables();

    void initializeSimulationModels();
    void syncSimulationModelsWithGraph();

    template<typename T>
    void setTableFromMatrix(QTableView *table, T &matrix, int height = -1, int width = -1);

    QActionGroup *nodeMovementGroup;
    QList<QTableView *> graphMatrixViews;
    QList<QTableView *> graphListViews;
    QMap<QString, QSpinBox *> graphCountSpins;

    // --- Указатели для новой боковой панели ---
    QDockWidget *dock_NodeInfo;
    NodeInfoWidget *nodeSidebar;

    QTimer *m_simTimer = nullptr;
    std::vector<std::shared_ptr<NodeModel>> m_nodeModels;

    ReliabilityWidget *m_reliabilityWidget = nullptr;
    QDockWidget *dock_Reliability = nullptr;

    QTabWidget *m_resultsTabs;
    ReliabilityChartWidget *m_chartWidget;
};
