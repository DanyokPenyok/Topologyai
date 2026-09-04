#include "mainwindow.h"
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QStandardItemModel>
#include <QStyle>
#include <QTextStream>
#include <QTimer>
#include <QToolTip>
#include "qspinbox.h"
#include "ui_mainwindow.h"

#include "logger.h"
#include "nodemodel.h"
#include "simulationengine.h"
#include "strategybasiccontrol.h"
#include "strategynocontrolcheck.h"
#include "strategyofflinecheck.h"

#include "node.h"
#include "nodeinfowidget.h"
#include "datasetgenerator.h"
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QProgressDialog>
#include <QDir>
#include <QProcess>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // =========================================================
    // СОЗДАЕМ DOCK WIDGET ДЛЯ БОКОВОЙ ПАНЕЛИ
    // =========================================================
    dock_NodeInfo = new QDockWidget("Node Settings", this);
    dock_NodeInfo->setObjectName("dock_NodeInfo");
    dock_NodeInfo->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    nodeSidebar = new NodeInfoWidget(this);
    dock_NodeInfo->setWidget(nodeSidebar);

    this->addDockWidget(Qt::RightDockWidgetArea, dock_NodeInfo);
    dock_NodeInfo->hide();
    // =========================================================

    // --- Настройка вкладок ---
    this->setDockOptions(this->dockOptions() & ~QMainWindow::VerticalTabs);
    this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    // --- Иконки и Подсказки ---
    ui->actionUndo->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    ui->actionUndo->setToolTip("Отменить последнее действие (Ctrl+Z)");

    ui->actionRedo->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    ui->actionRedo->setToolTip("Повторить отмененное действие (Ctrl+Y)");

    ui->actionSimulation->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->actionSimulation->setToolTip("Запустить симуляцию");

    ui->actionSave_as->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    ui->actionSave_as->setToolTip("Сохранить граф в файл (Ctrl+Shift+S)");

    ui->actionCopy->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->actionCopy->setToolTip("Копировать выбранные ячейки");

    ui->actionPaste->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
    ui->actionPaste->setToolTip("Вставить данные из буфера");

    ui->actionDeleteGraph->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    ui->actionDeleteGraph->setToolTip("Полностью удалить граф");

    ui->actionAddNode->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->actionAddNode->setToolTip("Добавить новый узел");

    ui->actionDeleteNode->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
    ui->actionDeleteNode->setToolTip("Удалить последний узел");

    ui->actionClearConsole->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    ui->actionClearConsole->setToolTip("Очистить историю в консоли");

    connect(ui->actionSave_as, &QAction::triggered, this, &MainWindow::saveToFile);

    connect(ui->actionAddNode, &QAction::triggered, this, [this]() {
        saveState();
        int newAmount = graph.getAmount() + 1;
        graph.resizeGraph(graph.getAmount(), newAmount);
        initializeSimulationModels();
        graph.graphView->initScene();
        if (m_simulation && m_simulation->isRunning())
            syncSimulationModelsWithGraph();
        updateTables();
        ui->textEdit_Console->appendPlainText("Добавлен узел. Всего: " + QString::number(newAmount));
    });

    connect(ui->actionDeleteNode, &QAction::triggered, this, [this]() {
        if (graph.getAmount() > 0) {
            saveState();
            nodeSidebar->setNode(nullptr);
            int newAmount = graph.getAmount() - 1;
            graph.resizeGraph(graph.getAmount(), newAmount);
            initializeSimulationModels();
            graph.graphView->scene()->update();
            if (m_simulation && m_simulation->isRunning())
                syncSimulationModelsWithGraph();
            updateTables();
            ui->textEdit_Console->appendPlainText("Узел удален. Осталось: " + QString::number(newAmount));
        }
    });

    connect(ui->actionDeleteGraph, &QAction::triggered, this, [this]() {
        saveState();
        nodeSidebar->setNode(nullptr);
        graph.resizeGraph(graph.getAmount(), 0);
        initializeSimulationModels();
        graph.graphView->scene()->update();
        if (m_simulation && m_simulation->isRunning())
            syncSimulationModelsWithGraph();
        updateTables();
        ui->textEdit_Console->appendPlainText("Граф полностью очищен.");
    });

    connect(ui->actionClearConsole, &QAction::triggered, this, &MainWindow::buttonClearConsoleClicked);

    connect(ui->actionUndo, &QAction::triggered, this, [this]() {
        if (!undoStack.empty()) {
            redoStack.append(captureState());
            restoreState(undoStack.takeLast());
            ui->textEdit_Console->appendPlainText("<- Undo");
        } else {
            ui->textEdit_Console->appendPlainText("Нечего отменять.");
        }
    });

    connect(ui->actionRedo, &QAction::triggered, this, [this]() {
        if (!redoStack.empty()) {
            undoStack.append(captureState());
            restoreState(redoStack.takeLast());
            ui->textEdit_Console->appendPlainText("-> Redo");
        } else {
            ui->textEdit_Console->appendPlainText("Нечего повторять.");
        }
    });

    connect(ui->actionSimulation, &QAction::triggered, this, [this]() {
        if (m_simulation && m_simulation->isRunning()) {
            onSimulationStop();
        } else {
            onSimulationStart();
        }
    });

    // Меню ИИ
    QMenu *menuAI = menuBar()->addMenu("ИИ");
    QAction *actionGenDataset = menuAI->addAction("Сгенерировать датасет...");
    actionGenDataset->setToolTip("Сгенерировать датасет");
    actionGenDataset->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(actionGenDataset, &QAction::triggered, this, &MainWindow::onGenerateAIDataset);

    QAction *actionExportCurrentAI = menuAI->addAction("Экспортировать сеть...");
    actionExportCurrentAI->setToolTip("Экспортировать сеть");
    actionExportCurrentAI->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(actionExportCurrentAI, &QAction::triggered, this, &MainWindow::onExportCurrentGraphAI);

    QAction *actionOptimizeAI = menuAI->addAction("Добавить ребро (ИИ)...");
    actionOptimizeAI->setToolTip("Добавить ребро ИИ");
    actionOptimizeAI->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(actionOptimizeAI, &QAction::triggered, this, &MainWindow::onOptimizeGraphAI);

    QAction *actionImportAI = menuAI->addAction("Загрузить рекомендацию...");
    actionImportAI->setToolTip("Загрузить рекомендацию");
    actionImportAI->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(actionImportAI, &QAction::triggered, this, &MainWindow::onImportAIRecommendation);

    ui->toolBar->addSeparator();
    ui->toolBar->addAction(actionGenDataset);
    ui->toolBar->addAction(actionExportCurrentAI);
    ui->toolBar->addAction(actionOptimizeAI);
    ui->toolBar->addAction(actionImportAI);

    auto spinBoxes = this->findChildren<QSpinBox *>();
    auto pushButtons = this->findChildren<QPushButton *>();

    for (auto *button: pushButtons) {
        if (button->text().contains("Clear") || button->objectName().contains("clear")) {
            connect(button, &QPushButton::clicked, this, &MainWindow::buttonClearConsoleClicked);
        }
    }

    for (auto *table: this->findChildren<QTableView *>()) {
        table->setAlternatingRowColors(true);
        if (table->objectName().endsWith("Graph")) {
            auto name = table->objectName().replace("table_", "").replace("_Graph", "");
            if (name.startsWith("Matrix")) {
                graphMatrixViews.append(table);
            } else if (name.startsWith("List")) {
                graphListViews.append(table);
                if (name.endsWith("Edges"))
                    table->setModel(new QStandardItemModel(0, 0));
            }

            for (auto *spinBox: spinBoxes) {
                if (spinBox->objectName().startsWith("spin_" + name) && spinBox->objectName().endsWith("NodesCount")) {
                    if (name.startsWith("Matrix")) {
                        connect(spinBox, &QSpinBox::valueChanged, this,
                                std::bind(&MainWindow::setNodesAmountMatrix, this, table, std::placeholders::_1));
                    }
                    graphCountSpins.insert(table->objectName(), spinBox);
                }
            }

            for (auto *button: pushButtons) {
                if (button->objectName().startsWith("button_" + name) && button->objectName().endsWith("_Apply")) {
                    if (name.startsWith("Matrix"))
                        connect(button, &QPushButton::clicked, this,
                                std::bind(&MainWindow::applyGraphMatrix, this, table));
                    else if (name == "ListEdges")
                        connect(button, &QPushButton::clicked, this,
                                std::bind(&MainWindow::applyEdgesList, this, table));
                }
            }
        }
        if (table->model() == nullptr)
            table->setModel(new QStandardItemModel(0, 0));
    }

    connect(ui->actionBandwidth, &QAction::triggered, this, [this](bool checked) {
        checked ? graph.setFlag(GraphFlags::ShowBandwidth) : graph.unsetFlag(GraphFlags::ShowBandwidth);
        graph.graphView->scene()->update();
    });

    connect(ui->actionWeights, &QAction::triggered, this, [this](bool checked) {
        checked ? graph.setFlag(GraphFlags::ShowWeights) : graph.unsetFlag(GraphFlags::ShowWeights);
        graph.graphView->scene()->update();
    });

    auto view = ui->menuView_mode;
    auto docks = this->findChildren<QDockWidget *>();
    if (!docks.empty()) {
        auto dockStack = this->findChild<QDockWidget *>("dock_PlaceHolder");
        QDockWidget *dockTop = nullptr;
        for (auto *dock: docks) {
            if (dock != dockStack) {
                auto act = new QAction(dock->windowTitle());
                act->setCheckable(true);

                // Скрываем чекбокс NodeInfo по умолчанию
                if (dock->objectName() == "dock_NodeInfo") {
                    act->setChecked(false);
                } else {
                    act->setChecked(true);
                    this->tabifyDockWidget(dockStack, dock);
                }

                if (!dockTop)
                    dockTop = dock;
                docksViewMode.insert(dock->windowTitle(), dock);
                connect(act, &QAction::triggered, this,
                        std::bind(&MainWindow::viewModeChecked, this, std::placeholders::_1));
                view->addAction(act);
            }
        }
        if (dockStack) {
            dockStack->hide();
            dockStack->setDisabled(true);
        }
        if (dockTop)
            dockTop->raise();
    }

    ui->centralwidget->layout()->removeWidget(ui->graphView);
    ui->centralwidget->layout()->addWidget(graph.graphView);

    connect(ui->actionCopy, &QAction::triggered, this, std::bind(&MainWindow::myCopy, this));
    connect(ui->actionPaste, &QAction::triggered, this, std::bind(&MainWindow::myPaste, this));

    nodeMovementGroup = new QActionGroup(this);
    nodeMovementGroup->addAction(ui->actionAutomatic);
    nodeMovementGroup->addAction(ui->actionManual);
    connect(ui->actionManual, &QAction::triggered, this, [this](bool checked) {
        if (checked)
            graph.setFlag(GraphFlags::ManualMode);
        graph.graphView->scene()->update();
    });
    connect(ui->actionAutomatic, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            graph.unsetFlag(GraphFlags::ManualMode);
            graph.graphView->runTimer();
        }
    });
    ui->actionAutomatic->setChecked(true);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(100);

    connect(m_simTimer, &QTimer::timeout, this, [this]() {
        if (m_simulation && m_simulation->isRunning()) {
            m_simulation->step();
            for (auto *item: graph.graphView->scene()->items()) {
                if (auto *node = qgraphicsitem_cast<Node *>(item)) {
                    node->update();
                }
            }
        }
    });

    Logger::registerCallback([this](const QString &msg) { ui->textEdit_Console->appendPlainText(msg); });

    initializeSimulationModels();
    m_resultsTabs = new QTabWidget(this);
    m_resultsTabs->setObjectName("resultsTabs");
    m_resultsTabs->setTabsClosable(false);

    m_reliabilityWidget = new ReliabilityWidget(this);
    m_reliabilityWidget->setGraph(&graph);
    m_resultsTabs->addTab(m_reliabilityWidget, "🔍 Структурный анализ");

    m_chartWidget = new ReliabilityChartWidget(this);
    m_resultsTabs->addTab(m_chartWidget, "📈 График надёжности");

    m_resultsTabs->setTabEnabled(1, false);
    m_resultsTabs->setTabToolTip(1, "Доступно после остановки симуляции");

    dock_Reliability = new QDockWidget("Результаты анализа", this);
    dock_Reliability->setObjectName("dock_Reliability");
    dock_Reliability->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dock_Reliability->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::BottomDockWidgetArea);
    dock_Reliability->setWidget(m_resultsTabs);

    this->addDockWidget(Qt::LeftDockWidgetArea, dock_Reliability);

    // Приклеиваем после последней матрицы (чтобы был внизу слева)
    if (!graphMatrixViews.isEmpty()) {
        for (auto *dock: findChildren<QDockWidget *>()) {
            if (dock->widget() == graphMatrixViews.last()) {
                this->tabifyDockWidget(dock, dock_Reliability);
                break;
            }
        }
    }
    dock_Reliability->raise();

    // Действие в меню для управления видимостью
    auto *reliabilityAct = new QAction("Результаты анализа", this);
    reliabilityAct->setCheckable(true);
    reliabilityAct->setChecked(true);
    connect(reliabilityAct, &QAction::triggered, this, [this](bool checked) { dock_Reliability->setVisible(checked); });
    ui->menuView_mode->addAction(reliabilityAct);
}

MainWindow::~MainWindow() {
    delete nodeMovementGroup;
    delete ui;
}

void MainWindow::saveToFile() {
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить граф", "", "Graph Files (*.gr);;All Files (*)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл для записи.");
        return;
    }

    QTextStream out(&file);
    unsigned int n = graph.getAmount();
    out << "Nodes: " << n << "\n\n";

    auto saveMatrix = [&](const QString &title, const Matrix2D &m) {
        out << title << ":\n";
        for (unsigned int i = 0; i < n; ++i) {
            for (unsigned int j = 0; j < n; ++j) {
                out << m[i][j] << (j == n - 1 ? "" : "\t");
            }
            out << "\n";
        }
        out << "\n";
    };

    saveMatrix("Bandwidth Matrix", graph.getMatrixBandwidth());

    file.close();
    ui->textEdit_Console->appendPlainText("Граф успешно сохранен в: " + fileName);
}

GraphState MainWindow::captureState() {
    GraphState s;
    s.amount = graph.getAmount();
    s.adj = graph.getMatrixAdjacent();
    s.flow = graph.getMatrixFlow();
    s.band = graph.getMatrixBandwidth();
    return s;
}

void MainWindow::saveState() {
    undoStack.append(captureState());
    redoStack.clear();
}

void MainWindow::restoreState(const GraphState &state) {
    nodeSidebar->setNode(nullptr);
    graph.resizeGraph(graph.getAmount(), state.amount);
    graph.setMatrixAdjacent(const_cast<Matrix2D &>(state.adj));
    graph.setMatrixFlow(const_cast<Matrix2D &>(state.flow));
    graph.setMatrixBandwidth(const_cast<Matrix2D &>(state.band));
    initializeSimulationModels();
    graph.graphView->initScene();
    updateTables();
}

void MainWindow::setUndoRedoEnabled(bool enabled) {
    ui->actionUndo->setEnabled(enabled);
    ui->actionRedo->setEnabled(enabled);
}

void MainWindow::buttonClearConsoleClicked() { ui->textEdit_Console->clear(); }

void MainWindow::pasteClipboardToTable(QTableView *dest) {
    if (!dest->hasFocus())
        return;
    QString text = QApplication::clipboard()->text();
    QStandardItemModel *model = static_cast<QStandardItemModel *>(dest->model());
    QModelIndexList indexes = dest->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;
    int row = indexes.first().row();
    int col = indexes.first().column();
    auto rows = text.split('\n', Qt::SkipEmptyParts);
    for (int i = 0; i < rows.size() && (row + i) < model->rowCount(); ++i) {
        auto cols = rows[i].split('\t');
        for (int j = 0; j < cols.size() && (col + j) < model->columnCount(); ++j) {
            model->item(row + i, col + j)->setText(cols[j]);
        }
    }
}

void MainWindow::viewModeChecked(bool checked) {
    auto *dock = docksViewMode[qobject_cast<QAction *>(sender())->text()];
    dock->setVisible(checked);
}

void MainWindow::setNodesAmountMatrix(QTableView *table, int newAmount) {
    QStandardItemModel *model = static_cast<QStandardItemModel *>(table->model());
    int oldAmount = model->columnCount();
    if (newAmount < 0)
        newAmount = 0;
    if (oldAmount < newAmount) {
        int delta = newAmount - oldAmount;
        model->insertColumns(oldAmount, delta);
        model->insertRows(oldAmount, delta);
        for (int i = oldAmount; i < newAmount; ++i) {
            model->setHeaderData(i, Qt::Horizontal, i + 1);
            model->setHeaderData(i, Qt::Vertical, i + 1);
            table->setColumnWidth(i, 25);
            table->setRowHeight(i, 25);
            for (int j = 0; j < newAmount; ++j) {
                model->setItem(i, j, new QStandardItem("0"));
                if (i != j)
                    model->setItem(j, i, new QStandardItem("0"));
            }
        }
    } else {
        model->setColumnCount(newAmount);
        model->setRowCount(newAmount);
    }
}

void MainWindow::copyTableToClipboard(QTableView *src) {
    if (!src->hasFocus())
        return;
    QModelIndexList indexes = src->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;
    std::sort(indexes.begin(), indexes.end());
    QString text;
    for (int i = 0; i < indexes.count(); ++i) {
        text.append(src->model()->data(indexes[i]).toString());
        if (i + 1 < indexes.count()) {
            text.append(indexes[i + 1].row() != indexes[i].row() ? "\n" : "\t");
        }
    }
    QApplication::clipboard()->setText(text);
}

void MainWindow::applyGraphMatrix(QTableView *table) {
    saveState();
    QStandardItemModel *model = static_cast<QStandardItemModel *>(table->model());
    int n = model->rowCount();
    Matrix2D m(n, QList<double>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            m[i][j] = model->item(i, j)->text().toDouble();
    }
    if (table->objectName().contains("Adj"))
        graph.setMatrixAdjacent(m);
    else if (table->objectName().contains("Flow"))
        graph.setMatrixFlow(m);
    else if (table->objectName().contains("Bandwidth"))
        graph.setMatrixBandwidth(m);
    graph.graphView->initScene();
    if (m_simulation && m_simulation->isRunning())
        syncSimulationModelsWithGraph();
    updateTables();
}

void MainWindow::applyEdgesList(QTableView *table) {
    saveState();
    auto model = static_cast<QStandardItemModel *>(table->model());
    for (int i = 0; i < model->rowCount(); ++i) {
        int u = model->item(i, 0)->text().toInt();
        int v = model->item(i, 1)->text().toInt();
        double w = model->item(i, 2)->text().toDouble();
        if (w > 0) {
            graph.setEdgeWeight(u, v, w);
            graph.setEdgeBandwidth(u, v, model->item(i, 3)->text().toDouble());
            graph.setEdgeFlow(u, v, model->item(i, 4)->text().toDouble());
        } else
            graph.removeEdge(u, v);
    }
    if (m_simulation && m_simulation->isRunning())
        syncSimulationModelsWithGraph();
    updateTables();
}

void MainWindow::updateEdgesList(QTableView *list) {
    auto edges = graph.getListEdges();
    auto model = static_cast<QStandardItemModel *>(list->model());
    model->setRowCount(edges.size());
    model->setColumnCount(5);
    QStringList headers = {"U", "V", "Weight", "Band", "Flow"};
    model->setHorizontalHeaderLabels(headers);
    for (int i = 0; i < edges.size(); i++) {
        for (int j = 0; j < 5; j++)
            model->setItem(i, j, new QStandardItem(edges[i][j].toString()));
    }
}

void MainWindow::updateTables() {
    unsigned int n = graph.getAmount();
    for (auto &spin: graphCountSpins)
        spin->setValue(n);
    for (auto &table: graphMatrixViews) {
        QString name = table->objectName();
        if (name.contains("Adj"))
            setTableFromMatrix(table, graph.getMatrixAdjacent(), n, n);
        else if (name.contains("Flow"))
            setTableFromMatrix(table, graph.getMatrixFlow(), n, n);
        else if (name.contains("Bandwidth"))
            setTableFromMatrix(table, graph.getMatrixBandwidth(), n, n);
    }
    for (auto &list: graphListViews)
        updateEdgesList(list);
}

void MainWindow::myCopy() {
    if (auto *t = qobject_cast<QTableView *>(focusWidget()))
        copyTableToClipboard(t);
}
void MainWindow::myPaste() {
    if (auto *t = qobject_cast<QTableView *>(focusWidget()))
        pasteClipboardToTable(t);
}

void MainWindow::initializeSimulationModels() {
    Logger::info("Инициализация моделей для симуляции");

    if (graph.getAmount() == 0) {
        Logger::info("Граф пуст. Добавьте узлы перед запуском симуляции.");
        ui->textEdit_Console->appendPlainText("⚠ Сначала добавьте узлы (кнопка + или N)");
        return;
    }

    syncSimulationModelsWithGraph();
}

void MainWindow::syncSimulationModelsWithGraph() {
    QMap<unsigned int, std::shared_ptr<NodeModel>> existingModels;
    for (const auto &model: m_nodeModels) {
        existingModels.insert(model->id(), model);
    }

    const auto &nodesMap = graph.getNodes();
    m_nodeModels.clear();
    m_nodeModels.reserve(nodesMap.size());

    for (auto it = nodesMap.constBegin(); it != nodesMap.constEnd(); ++it) {
        unsigned int id = it.key();
        Node *qtNode = it.value();

        auto model = existingModels.value(id);
        if (!model) {
            model = std::make_shared<NodeModel>(id);

            // циклически для демонстрации
            switch (id % 3) {
                case 0:
                    model->setStrategy(std::make_unique<StrategyBasicControl>());
                    break;
                case 1:
                    model->setStrategy(std::make_unique<StrategyNoControlCheck>());
                    break;
                default:
                    model->setStrategy(std::make_unique<StrategyOfflineCheck>());
                    break;
            }
        }

        m_nodeModels.push_back(model);
        qtNode->bindModel(model.get());

        Logger::info(
                QString("Node-%1: стратегия %2").arg(id).arg(model->strategy() ? model->strategy()->name() : "None"));
    }

    if (!m_simulation) {
        m_simulation = std::make_unique<SimulationEngine>(0.1, "exponential");
        m_simulation->setEventCallback([this](unsigned int nodeId, const std::string &event) {
            Logger::event(nodeId, QString::fromStdString(event));
        });
        m_simulation->setRecordHistory(true);
    }
    m_simulation->setNodes(m_nodeModels);

    Logger::info(QString("Готово: %1 узлов привязано").arg(m_nodeModels.size()));
    graph.graphView->scene()->update();
}

void MainWindow::onSimulationStart() {
    if (m_simulation && m_simulation->isRunning()) {
        Logger::info("Симуляция уже запущена.");
        return;
    }

    if (!m_simulation || m_nodeModels.empty() || graph.getAmount() != m_nodeModels.size()) {
        initializeSimulationModels();
        if (!m_simulation)
            return;
    }

    if (m_resultsTabs) {
        m_resultsTabs->setTabEnabled(1, false);
        m_resultsTabs->setTabToolTip(1, "Доступно после остановки симуляции");
        if (m_chartWidget)
            m_chartWidget->clear();
    }

    Logger::info("Запуск имитационного моделирования");
    m_simulation->start();
    m_simTimer->start();
    setUndoRedoEnabled(false);
    ui->actionSimulation->setText("Stop Simulation");
    ui->actionSimulation->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->actionSimulation->setToolTip("Остановить симуляцию");
    ui->textEdit_Console->appendPlainText("Симуляция запущена");
}

void MainWindow::onSimulationStop() {
    if (m_simulation) {
        m_simulation->stop();
        m_simTimer->stop();

        if (m_chartWidget && m_simulation) {
            m_chartWidget->updateChart(m_simulation.get());
            m_resultsTabs->setTabEnabled(1, true);
            m_resultsTabs->setTabToolTip(1, "");
            m_resultsTabs->setCurrentIndex(1);
        }

        setUndoRedoEnabled(true);
        ui->actionSimulation->setText("Start Simulation");
        ui->actionSimulation->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        ui->actionSimulation->setToolTip("Запустить симуляцию");

        double avail = m_simulation->getGlobalAvailability();
        Logger::info(QString("Симуляция остановлена. Kг = %1").arg(avail, 0, 'f', 4));

        ui->textEdit_Console->appendPlainText(
                QString("Симуляция остановлена. Коэффициент готовности: %1").arg(avail, 0, 'f', 4));
    }
    onSimulationStopped();
}

void MainWindow::onSimulationStopped() { Logger::info("Пост-обработка после остановки симуляции"); }

bool MainWindow::saveSimulationResults(const QString &fileName) {


    if (m_nodeModels.empty()) {
        return false;
    }

    bool result = m_simulation->exportStatsToCSV(fileName);

    if (result) {
        Logger::info(QString("Экспорт завершён: %1").arg(fileName));
    }

    return result;
}

template<typename T>
void MainWindow::setTableFromMatrix(QTableView *table, T &matrix, int h, int w) {
    auto *model = static_cast<QStandardItemModel *>(table->model());
    model->setRowCount(h);
    model->setColumnCount(w);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (!model->item(i, j))
                model->setItem(i, j, new QStandardItem());
            model->item(i, j)->setText(QString::number(matrix[i][j]));
        }
    }
}

void MainWindow::onGenerateAIDataset() {
    QDialog dialog(this);
    dialog.setWindowTitle("Генерация датасета");
    dialog.setMinimumWidth(380);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel("<b>Параметры генерации:</b>", &dialog));

    auto *formLayout = new QFormLayout();
    auto *samplesSpin = new QSpinBox(&dialog);
    samplesSpin->setRange(5, 50000);
    samplesSpin->setValue(500);
    formLayout->addRow("Количество сетей:", samplesSpin);

    auto *minNodesSpin = new QSpinBox(&dialog);
    minNodesSpin->setRange(3, 100);
    minNodesSpin->setValue(10);
    formLayout->addRow("Мин. узлов в сети:", minNodesSpin);

    auto *maxNodesSpin = new QSpinBox(&dialog);
    maxNodesSpin->setRange(3, 100);
    maxNodesSpin->setValue(20);
    formLayout->addRow("Макс. узлов в сети:", maxNodesSpin);

    auto *warmupSpin = new QSpinBox(&dialog);
    warmupSpin->setRange(10, 5000);
    warmupSpin->setValue(150);
    warmupSpin->setToolTip("Шаги симуляции");
    formLayout->addRow("Количество шагов:", warmupSpin);

    layout->addLayout(formLayout);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    int count = samplesSpin->value();
    int minN = minNodesSpin->value();
    int maxN = maxNodesSpin->value();
    int warmupSteps = warmupSpin->value();
    if (minN > maxN) std::swap(minN, maxN);

    QString defaultPath = QDir::current().filePath("dataset.json");
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить датасет", defaultPath, "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QProgressDialog progress("Генерация сетей для датасета...", "Отмена", 0, count, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setValue(0);

    ui->textEdit_Console->appendPlainText(QString("Начало генерации датасета (%1 сетей, от %2 до %3 узлов, шагов: %4)...").arg(count).arg(minN).arg(maxN).arg(warmupSteps));
    QApplication::processEvents();

    DatasetGenerator generator(warmupSteps);
    bool canceled = false;
    bool success = generator.saveDatasetToFile(fileName, count, minN, maxN, [&](int current, int total) {
        if (current % 20 == 0 || current == total) {
            progress.setValue(current);
            QApplication::processEvents();
            if (progress.wasCanceled()) {
                canceled = true;
            }
        }
    });

    if (canceled) {
        ui->textEdit_Console->appendPlainText("⚠ Генерация датасета прервана пользователем.");
    } else if (success) {
        ui->textEdit_Console->appendPlainText(QString("✓ Датасет успешно сохранён: %1 (%2 сетей)").arg(fileName).arg(count));
        QMessageBox::information(this, "Успех", QString("Датасет из %1 сетей успешно создан и сохранён в:\n%2").arg(count).arg(fileName));
    } else {
        ui->textEdit_Console->appendPlainText("❌ Ошибка при сохранении датасета в файл.");
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить датасет в указанный файл.");
    }
}

void MainWindow::onExportCurrentGraphAI() {
    if (graph.getAmount() == 0) {
        QMessageBox::warning(this, "Внимание", "Граф пуст! Добавьте узлы и рёбра перед экспортом.");
        return;
    }

    QString defaultPath = QDir::current().filePath("current_network_for_ai.json");
    QString fileName = QFileDialog::getSaveFileName(this, "Экспорт текущей сети для ИИ", defaultPath, "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    DatasetGenerator generator;
    bool success = generator.saveCurrentGraphToFile(fileName, graph, 0, static_cast<int>(graph.getAmount()) - 1);
    if (success) {
        DatasetSample sample = generator.sampleFromGraph(graph, 0, static_cast<int>(graph.getAmount()) - 1);
        QString msg = QString("✓ Сеть экспортирована в: %1\nУзлов: %2, Рёбер: %3\nБазовая надёжность: %4\n")
            .arg(fileName).arg(sample.numNodes).arg(sample.numEdges).arg(sample.baseReliability, 0, 'f', 4);
        if (sample.bestEdgeToAdd.first != -1) {
            msg += QString("Лучшее рекомендуемое ребро: (%1 -> %2)\nОжидаемый прирост: +%3 (итоговая надёжность: %4)")
                .arg(sample.bestEdgeToAdd.first)
                .arg(sample.bestEdgeToAdd.second)
                .arg(sample.bestGain, 0, 'f', 4)
                .arg(sample.bestReliability, 0, 'f', 4);
        } else {
            msg += "Дополнительных связей, увеличивающих связность исток-сток, не найдено.";
        }
        ui->textEdit_Console->appendPlainText(msg);
        QMessageBox::information(this, "Экспорт завершён", msg);
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось записать файл.");
    }
}

void MainWindow::onOptimizeGraphAI() {
    if (graph.getAmount() < 2) {
        QMessageBox::warning(this, "Внимание", "Добавьте узлы");
        return;
    }

    QString tempGraph = QDir::current().filePath("current_network_for_ai.json");
    QString resultFile = QDir::current().filePath("ai_recommendation.json");

    DatasetGenerator generator;
    if (!generator.saveCurrentGraphToFile(tempGraph, graph, 0, static_cast<int>(graph.getAmount()) - 1)) {
        QMessageBox::warning(this, "Ошибка", "Ошибка экспорта");
        return;
    }

    QString pythonPath = "C:/Users/danya/PycharmProjects/topologyMLdl/.venv/Scripts/python.exe";
    if (!QFile::exists(pythonPath)) {
        pythonPath = "python";
    }

    QString scriptPath = QDir::current().filePath("main.py");
    if (!QFile::exists(scriptPath)) {
        scriptPath = "C:/Users/danya/PycharmProjects/topologyMLdl/main.py";
    }

    QString modelPath = QDir::current().filePath("best_model.pth");
    if (!QFile::exists(modelPath)) {
        modelPath = "C:/Users/danya/PycharmProjects/topologyMLdl/best_model.pth";
    }

    if (!QFile::exists(modelPath)) {
        QMessageBox::warning(this, "Ошибка", "Модель не найдена");
        return;
    }

    ui->textEdit_Console->appendPlainText(QString("Запуск ИИ на GPU (поиск пути 0 -> %1)...").arg(graph.getAmount() - 1));
    QApplication::processEvents();

    QProcess process;
    QStringList args;
    args << scriptPath << "--predict" << tempGraph << "--model" << modelPath << "--output" << resultFile;
    process.start(pythonPath, args);
    if (!process.waitForFinished(30000)) {
        ui->textEdit_Console->appendPlainText("Таймаут работы ИИ.");
        QMessageBox::warning(this, "Ошибка", "ИИ не ответил");
        return;
    }

    if (process.exitCode() != 0) {
        QString err = QString::fromUtf8(process.readAllStandardError());
        ui->textEdit_Console->appendPlainText("Ошибка ИИ: " + err);
        QMessageBox::warning(this, "Ошибка ИИ", err.isEmpty() ? "Сбой инференса" : err);
        return;
    }

    applyRecommendationJson(resultFile);
}

void MainWindow::onImportAIRecommendation() {
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть рекомендацию", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    applyRecommendationJson(fileName);
}

void MainWindow::applyRecommendationJson(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Файл не открыт");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QJsonArray edgeArr = obj["recommended_best_edge"].toArray();
    if (edgeArr.size() < 2) {
        QMessageBox::warning(this, "Внимание", "Рекомендаций нет");
        return;
    }

    int u = edgeArr[0].toInt();
    int v = edgeArr[1].toInt();
    double gain = obj["predicted_gain"].toDouble();

    if (u < 0 || v < 0 || u >= static_cast<int>(graph.getAmount()) || v >= static_cast<int>(graph.getAmount())) {
        QMessageBox::warning(this, "Ошибка", "Некорректные узлы");
        return;
    }

    int targetSink = static_cast<int>(graph.getAmount()) - 1;
    QString msg = QString("Оптимизация пути: узел 0 ➔ узел %1\n\n"
                          "ИИ рекомендует добавить связь:\n"
                          "Узлы: (%2 <---> %3)\n"
                          "Ожидаемый прирост надёжности: +%4")
        .arg(targetSink).arg(u).arg(v).arg(gain, 0, 'f', 4);

    QMessageBox box(this);
    box.setWindowTitle("Рекомендация ИИ");
    box.setIcon(QMessageBox::Information);
    box.setText(msg);
    QPushButton *addBtn = box.addButton("Добавить ребро", QMessageBox::AcceptRole);
    box.addButton("Отмена", QMessageBox::RejectRole);
    box.setDefaultButton(addBtn);
    box.exec();

    if (box.clickedButton() == addBtn) {
        try {
            saveState();

            Matrix2D adj = graph.getMatrixAdjacent();
            Matrix2D band = graph.getMatrixBandwidth();

            adj[u][v] = 1.0;
            adj[v][u] = 1.0;
            if (band[u][v] <= 0) band[u][v] = 1.0;
            if (band[v][u] <= 0) band[v][u] = 1.0;

            graph.setMatrixAdjacent(adj);
            graph.setMatrixBandwidth(band);
            graph.graphView->initScene();

            if (m_simulation && m_simulation->isRunning())
                syncSimulationModelsWithGraph();

            updateTables();

            QString consoleMsg = QString("✓ ИИ: ребро (%1 <-> %2) добавлено (+%3)").arg(u).arg(v).arg(gain, 0, 'f', 4);
            ui->textEdit_Console->appendPlainText(consoleMsg);
            QMessageBox::information(this, "Успех", "Ребро добавлено");
        } catch (const std::exception &e) {
            ui->textEdit_Console->appendPlainText(QString("Ошибка добавления ребра: %1").arg(e.what()));
            QMessageBox::warning(this, "Ошибка", "Сбой добавления");
        }
    }
}

