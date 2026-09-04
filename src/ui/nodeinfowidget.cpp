#include "nodeinfowidget.h"
#include "edge.h"
#include "node.h"
#include "strategybasiccontrol.h"
#include "strategyfixedintervalcontrol.h"
#include "strategyinstantcheckdetection.h"
#include "strategyinstantdetection.h"
#include "strategynocontrolcheck.h"
#include "strategyofflinecheck.h"
#include "strategyperiodiccontrolemergencyrecovery.h"
#include "strategypreventivewithcontrol.h"

#include <QButtonGroup>
#include <QFormLayout>
#include <QFrame>
#include <QVBoxLayout>

NodeInfoWidget::NodeInfoWidget(QWidget *parent) : QWidget(parent) {
    setObjectName("NodeInfoWidget");

    setStyleSheet(
            "#NodeInfoWidget { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #f8fafc, stop:1 #eef2f7); }"
            "QLabel { font-size: 13px; border: none; }"
            "QLabel#titleLabel { font-size: 15px; font-weight: 700; }"
            "QLabel#stateBadge { border-radius: 10px; padding: 2px 10px; font-weight: 600; }"
            "QPlainTextEdit { border: 1px; border-radius: 8px; padding: 6px; "
            "font-family: Consolas, 'Courier New', monospace; font-size: 12px; }"
            "QCheckBox { font-size: 13px; spacing: 8px; padding: 4px; }"
            "QCheckBox::indicator { width: 15px; height: 15px; }"
            "QCheckBox::indicator:unchecked { border: 1px; border-radius: 4px; background: #fff; }"
            "QCheckBox::indicator:checked { border: 1px; border-radius: 4px; background: #2563eb; }"
            "QDoubleSpinBox { padding: 4px; border: 1px solid #cbd5e1; border-radius: 4px; }");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    auto *titleLabel = new QLabel("Node Inspector", this);
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);

    summaryValue = new QLabel("Выберите вершину на графе", this);
    summaryValue->setStyleSheet("color: #475569; font-size: 12px;");
    mainLayout->addWidget(summaryValue);

    auto *divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: #cbd5e1;");
    mainLayout->addWidget(divider);

    auto *form = new QFormLayout();
    form->setSpacing(8);

    idValue = new QLabel("-", this);
    stateValue = new QLabel("-", this);
    reliabilityValue = new QLabel("-", this);
    deliveryValue = new QLabel("-", this);
    deliveryValue->setToolTip("Надёжность дохода данных (по входящим связям)");
    edgeListValue = new QPlainTextEdit(this);
    inboxValue = new QPlainTextEdit(this);
    edgeListValue->setReadOnly(true);
    inboxValue->setReadOnly(true);
    edgeListValue->setMaximumHeight(90);
    inboxValue->setMaximumHeight(130);

    form->addRow("id:", idValue);
    form->addRow("state:", stateValue);
    form->addRow("reliability:", reliabilityValue);
    form->addRow("delivery:", deliveryValue);
    form->addRow("edgeList:", edgeListValue);
    form->addRow("inbox:", inboxValue);
    mainLayout->addLayout(form);

    auto *strategyLabel = new QLabel("Strategy (single select):", this);
    strategyLabel->setStyleSheet("font-weight: bold; margin-top: 8px;");
    mainLayout->addWidget(strategyLabel);

    strategyGroup = new QButtonGroup(this);
    strategyGroup->setExclusive(true);

    auto addStrategyCheck = [this, mainLayout](const QString &text) -> QCheckBox * {
        auto *check = new QCheckBox(text, this);
        strategyGroup->addButton(check);
        mainLayout->addWidget(check);
        return check;
    };

    strategyBasicControlCheck = addStrategyCheck("1.1 BasicControl");
    strategyInstantDetectionCheck = addStrategyCheck("1.2 InstantDetection");
    strategyPreventiveWithControlCheck = addStrategyCheck("2.1 Preventive+Control");
    strategyNoControlCheckCheck = addStrategyCheck("2.2 NoControlCheck");
    strategyInstantCheckDetectionCheck = addStrategyCheck("2.3 InstantCheckDetection");
    strategyOfflineCheckCheck = addStrategyCheck("2.4 OfflineCheck");
    strategyFixedIntervalControlCheck = addStrategyCheck("3. FixedIntervalControl");
    strategyPeriodicControlEmergencyRecoveryCheck = addStrategyCheck("4. PeriodicControl+Emergency");

    paramsContainer = new QWidget(this);
    paramsContainer->setStyleSheet("background: #f1f5f9; border-radius: 6px; padding: 8px;");
    paramsLayout = new QVBoxLayout(paramsContainer);
    paramsLayout->setContentsMargins(8, 8, 8, 8);
    paramsLayout->setSpacing(6);

    auto *paramsTitle = new QLabel("Parameters:", paramsContainer);
    paramsTitle->setStyleSheet("font-weight: bold;");
    paramsLayout->addWidget(paramsTitle);

    mainLayout->addWidget(paramsContainer);
    paramsContainer->hide();

    m_distGroupBox = new QGroupBox("Распределение времени событий", this);
    auto *distLayout = new QFormLayout(m_distGroupBox);
    distLayout->setSpacing(6);

    m_distTypeCombo = new QComboBox(this);
    m_distTypeCombo->addItem("Экспоненциальное", static_cast<int>(NodeModel::DistributionType::Exponential));
    m_distTypeCombo->addItem("Нормальное", static_cast<int>(NodeModel::DistributionType::Normal));
    connect(m_distTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &NodeInfoWidget::onDistributionTypeChanged);

    m_expRateSpin = new QDoubleSpinBox(this);
    m_expRateSpin->setRange(0.01, 10.0);
    m_expRateSpin->setSingleStep(0.01);
    m_expRateSpin->setDecimals(2);
    m_expRateSpin->setValue(0.1);
    m_expRateSpin->setToolTip("λ: интенсивность (среднее время = 1/λ)");
    connect(m_expRateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &NodeInfoWidget::onDistributionParamsChanged);

    m_normMeanSpin = new QDoubleSpinBox(this);
    m_normMeanSpin->setRange(0.1, 100.0);
    m_normMeanSpin->setSingleStep(0.5);
    m_normMeanSpin->setValue(10.0);
    m_normMeanSpin->setToolTip("μ: среднее время");
    connect(m_normMeanSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &NodeInfoWidget::onDistributionParamsChanged);

    m_normStdSpin = new QDoubleSpinBox(this);
    m_normStdSpin->setRange(0.1, 20.0);
    m_normStdSpin->setSingleStep(0.1);
    m_normStdSpin->setValue(2.0);
    m_normStdSpin->setToolTip("σ: стандартное отклонение");
    connect(m_normStdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &NodeInfoWidget::onDistributionParamsChanged);

    m_normMeanSpin->setVisible(false);
    m_normStdSpin->setVisible(false);

    distLayout->addRow("Тип:", m_distTypeCombo);
    distLayout->addRow("λ (эксп.):", m_expRateSpin);
    distLayout->addRow("μ (норм.):", m_normMeanSpin);
    distLayout->addRow("σ (норм.):", m_normStdSpin);

    mainLayout->addWidget(m_distGroupBox);

    mainLayout->addStretch();

    for (auto *button: strategyGroup->buttons()) {
        connect(button, &QCheckBox::toggled, this, &NodeInfoWidget::onStrategyToggled);
    }

    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(250);
    connect(refreshTimer, &QTimer::timeout, this, &NodeInfoWidget::refreshView);
    refreshTimer->start();

    clearView();
}

void NodeInfoWidget::setNode(Node *node) {
    currentNode = node;
    refreshView();
}

void NodeInfoWidget::onStrategyToggled(bool checked) {
    if (guardStrategyUpdate || !currentNode || !currentNode->model())
        return;

    auto *senderCheck = qobject_cast<QCheckBox *>(sender());
    if (!senderCheck || !checked)
        return;

    guardStrategyUpdate = true;

    for (auto *btn: strategyGroup->buttons()) {
        if (btn != senderCheck)
            btn->setChecked(false);
    }

    currentNode->model()->setStrategy(createStrategyFor(senderCheck));

    updateParamsVisibility();

    guardStrategyUpdate = false;
}

void NodeInfoWidget::refreshView() {
    if (!currentNode || !currentNode->model()) {
        clearView();
        return;
    }

    const auto model = currentNode->model();
    idValue->setText(QString::number(model->id()));

    switch (model->state()) {
        case NodeModel::State::Operational:
            stateValue->setText("Operational");
            stateValue->setStyleSheet(
                    "background: #dcfce7; color: #166534; border-radius: 10px; padding: 2px 10px; font-weight: 600;");
            break;
        case NodeModel::State::Failed:
            stateValue->setText("Failed");
            stateValue->setStyleSheet(
                    "background: #fee2e2; color: #991b1b; border-radius: 10px; padding: 2px 10px; font-weight: 600;");
            break;
        case NodeModel::State::Maintenance:
            stateValue->setText("Maintenance");
            stateValue->setStyleSheet(
                    "background: #fef3c7; color: #92400e; border-radius: 10px; padding: 2px 10px; font-weight: 600;");
            break;
    }

    reliabilityValue->setText(QString::number(model->reliability(), 'f', 4));

    // Надёжность дохода данных
    int inCount = 0;
    double unreliability = 1.0;
    for (auto *edge: currentNode->edges()) {
        if (edge->destNode() == currentNode && edge->sourceNode() != currentNode) {
            inCount++;
            double srcRel = (edge->sourceNode() && edge->sourceNode()->model())
                            ? edge->sourceNode()->model()->reliability()
                            : 0.95;
            double pathRel = srcRel * 0.98;
            unreliability *= (1.0 - pathRel);
        }
    }

    if (inCount == 0) {
        deliveryValue->setText("0 (нет входа)");
        deliveryValue->setStyleSheet("color: #dc2626; font-weight: 600;");
    } else {
        double deliveryRel = 1.0 - unreliability;
        deliveryValue->setText(QString::number(deliveryRel, 'f', 4));
        deliveryValue->setStyleSheet("color: #166534; font-weight: 600;");
    }

    QStringList edges;
    for (auto *edge: currentNode->edges()) {
        edges << QString("%1 -> %2").arg(edge->sourceNode()->getIndex()).arg(edge->destNode()->getIndex());
    }
    edgeListValue->setPlainText(edges.isEmpty() ? "-" : edges.join('\n'));

    QStringList inboxMessages;
    const auto &inbox = model->inbox();
    inboxMessages.reserve(static_cast<int>(inbox.size()));
    for (const auto &message: inbox) {
        inboxMessages << QString::fromStdString(message);
    }
    inboxValue->setPlainText(inboxMessages.isEmpty() ? "-" : inboxMessages.join('\n'));

    summaryValue->setText(
            QString("Node #%1 | edges: %2 | inbox: %3").arg(model->id()).arg(edges.size()).arg(inboxMessages.size()));

    if (currentNode && currentNode->model()) {
        const auto &params = currentNode->model()->getDistributionParams();
        const bool isNormal = (params.type == NodeModel::DistributionType::Normal);

        guardDistUpdate = true;

        if (const int index = isNormal ? 1 : 0; m_distTypeCombo->currentIndex() != index)
            m_distTypeCombo->setCurrentIndex(index);

        if (!isNormal) {
            if (m_expRateSpin->value() != params.expRate)
                m_expRateSpin->setValue(params.expRate);

        } else {
            if (m_normMeanSpin->value() != params.normMean)
                m_normMeanSpin->setValue(params.normMean);
            if (m_normStdSpin->value() != params.normStd)
                m_normStdSpin->setValue(params.normStd);
        }
        m_expRateSpin->setVisible(!isNormal);
        m_normMeanSpin->setVisible(isNormal);
        m_normStdSpin->setVisible(isNormal);

        if (m_distGroupBox)
            m_distGroupBox->setEnabled(true);

        guardDistUpdate = false;
    } else {
        if (m_distGroupBox)
            m_distGroupBox->setEnabled(false);
    }

    syncStrategyChecks();
    updateParamsVisibility();
}

void NodeInfoWidget::onDistributionTypeChanged(int index) {
    if (!currentNode || !currentNode->model())
        return;

    const auto type = static_cast<NodeModel::DistributionType>(m_distTypeCombo->itemData(index).toInt());
    currentNode->model()->setDistributionType(type);

    const bool isNormal = (type == NodeModel::DistributionType::Normal);
    m_expRateSpin->setVisible(!isNormal);
    m_normMeanSpin->setVisible(isNormal);
    m_normStdSpin->setVisible(isNormal);

    const auto &params = currentNode->model()->getDistributionParams();
    if (isNormal) {
        m_normMeanSpin->setValue(params.normMean);
        m_normStdSpin->setValue(params.normStd);
    } else {
        m_expRateSpin->setValue(params.expRate);
    }
}

void NodeInfoWidget::onDistributionParamsChanged() {
    if (!currentNode || !currentNode->model())
        return;

    auto type = static_cast<NodeModel::DistributionType>(m_distTypeCombo->currentData().toInt());
    if (type == NodeModel::DistributionType::Normal) {
        currentNode->model()->setNormalParams(m_normMeanSpin->value(), m_normStdSpin->value());
    } else {
        currentNode->model()->setExpRate(m_expRateSpin->value());
    }
}

void NodeInfoWidget::clearView() {
    idValue->setText("-");
    stateValue->setText("-");
    stateValue->setStyleSheet(
            "background: #e2e8f0; color: #475569; border-radius: 10px; padding: 2px 10px; font-weight: 600;");
    reliabilityValue->setText("-");
    deliveryValue->setText("-");
    deliveryValue->setStyleSheet("");
    summaryValue->setText("Выберите вершину на графе");
    edgeListValue->setPlainText("-");
    inboxValue->setPlainText("-");

    guardStrategyUpdate = true;
    for (auto *btn: strategyGroup->buttons()) {
        btn->setChecked(false);
        btn->setEnabled(false);
    }
    guardStrategyUpdate = false;

    paramsContainer->hide();
    for (auto &widgets: strategyParamsWidgets) {
        for (auto *w: widgets)
            w->hide();
    }

    guardDistUpdate = true;
    if (m_distTypeCombo)
        m_distTypeCombo->setCurrentIndex(0);
    if (m_expRateSpin)
        m_expRateSpin->setValue(0.1);
    if (m_normMeanSpin)
        m_normMeanSpin->setValue(10.0);
    if (m_normStdSpin)
        m_normStdSpin->setValue(2.0);
    if (m_expRateSpin)
        m_expRateSpin->setVisible(true);
    if (m_normMeanSpin)
        m_normMeanSpin->setVisible(false);
    if (m_normStdSpin)
        m_normStdSpin->setVisible(false);
    guardDistUpdate = false;

    if (m_distGroupBox)
        m_distGroupBox->setEnabled(currentNode != nullptr && currentNode->model() != nullptr);

    strategyParamsWidgets.clear();
}

void NodeInfoWidget::syncStrategyChecks() {
    auto *model = currentNode ? currentNode->model() : nullptr;
    const auto *strategy = model ? model->strategy() : nullptr;

    const bool enabled = (model != nullptr);

    for (auto *btn: strategyGroup->buttons()) {
        btn->setEnabled(enabled);
    }

    guardStrategyUpdate = true;
    for (auto *btn: strategyGroup->buttons()) {
        btn->setChecked(false);
    }

    if (strategy) {
        if (dynamic_cast<const StrategyBasicControl *>(strategy))
            strategyBasicControlCheck->setChecked(true);
        else if (dynamic_cast<const StrategyInstantDetection *>(strategy))
            strategyInstantDetectionCheck->setChecked(true);
        else if (dynamic_cast<const StrategyPreventiveWithControl *>(strategy))
            strategyPreventiveWithControlCheck->setChecked(true);
        else if (dynamic_cast<const StrategyNoControlCheck *>(strategy))
            strategyNoControlCheckCheck->setChecked(true);
        else if (dynamic_cast<const StrategyInstantCheckDetection *>(strategy))
            strategyInstantCheckDetectionCheck->setChecked(true);
        else if (dynamic_cast<const StrategyOfflineCheck *>(strategy))
            strategyOfflineCheckCheck->setChecked(true);
        else if (dynamic_cast<const StrategyFixedIntervalControl *>(strategy))
            strategyFixedIntervalControlCheck->setChecked(true);
        else if (dynamic_cast<const StrategyPeriodicControlEmergencyRecovery *>(strategy))
            strategyPeriodicControlEmergencyRecoveryCheck->setChecked(true);
    }
    guardStrategyUpdate = false;
}

std::unique_ptr<IMaintenanceStrategy> NodeInfoWidget::createStrategyFor(QObject *checkBox) const {
    if (checkBox == strategyBasicControlCheck) {
        return std::make_unique<StrategyBasicControl>();
    }
    if (checkBox == strategyInstantDetectionCheck) {
        return std::make_unique<StrategyInstantDetection>();
    }
    if (checkBox == strategyPreventiveWithControlCheck) {
        auto s = std::make_unique<StrategyPreventiveWithControl>(10.0);
        return s;
    }
    if (checkBox == strategyNoControlCheckCheck) {
        return std::make_unique<StrategyNoControlCheck>();
    }
    if (checkBox == strategyInstantCheckDetectionCheck) {
        auto s = std::make_unique<StrategyInstantCheckDetection>(5.0, 0.2);
        return s;
    }
    if (checkBox == strategyOfflineCheckCheck) {
        auto s = std::make_unique<StrategyOfflineCheck>(10.0);
        return s;
    }
    if (checkBox == strategyFixedIntervalControlCheck) {
        auto s = std::make_unique<StrategyFixedIntervalControl>(8.0);
        return s;
    }
    if (checkBox == strategyPeriodicControlEmergencyRecoveryCheck) {
        auto s = std::make_unique<StrategyPeriodicControlEmergencyRecovery>(5.0);
        return s;
    }
    return std::make_unique<StrategyBasicControl>(); // fallback
}

QDoubleSpinBox *NodeInfoWidget::createDoubleParam(const QString &label, double min, double max, double step,
                                                  double value) {
    auto *container = new QWidget(paramsContainer);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *lbl = new QLabel(label + ":", container);
    lbl->setFixedWidth(120);

    auto *spin = new QDoubleSpinBox(container);
    spin->setRange(min, max);
    spin->setSingleStep(step);
    spin->setValue(value);
    spin->setDecimals(2);

    layout->addWidget(lbl);
    layout->addWidget(spin);
    layout->addStretch();

    paramsLayout->addWidget(container);
    return spin;
}

QLabel *NodeInfoWidget::createReadOnlyParam(const QString &label, const QString &value) {
    auto *container = new QWidget(paramsContainer);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *lbl = new QLabel(label + ":", container);
    lbl->setFixedWidth(140);
    lbl->setStyleSheet("color: #475569;");

    auto *valLabel = new QLabel(value, container);
    valLabel->setStyleSheet(
            "color: #64748b; font-style: italic; background: #f1f5f9; padding: 2px 6px; border-radius: 3px;");
    valLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    layout->addWidget(lbl);
    layout->addWidget(valLabel, 1);
    layout->addStretch();

    paramsLayout->addWidget(container);
    return valLabel;
}

void NodeInfoWidget::updateParamsVisibility() {
    QLayoutItem *child;
    while ((child = paramsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    strategyParamsWidgets.clear();

    if (!currentNode || !currentNode->model()) {
        paramsContainer->hide();
        return;
    }

    auto *strategy = currentNode->model()->strategy();
    if (!strategy) {
        paramsContainer->hide();
        return;
    }

    QList<QWidget *> params;
    QString strategyName = QString::fromUtf8(strategy->name());

    if (auto *s = dynamic_cast<StrategyBasicControl *>(strategy)) {
        params.append(createReadOnlyParam("Detect failure range", "2.0 – 4.0"));
        params.append(createReadOnlyParam("Recover range", "3.0 – 6.0"));
        params.append(createReadOnlyParam("Preventive maintenance", "Not supported"));
        params.append(createReadOnlyParam("Distribution type", "Uniform"));
    } else if (auto *s = dynamic_cast<StrategyInstantDetection *>(strategy)) {
        params.append(createReadOnlyParam("Detect failure", "0.0 (instant)"));
        params.append(createReadOnlyParam("Recover range", "2.0 – 4.0"));
        params.append(createReadOnlyParam("Preventive maintenance", "Not supported"));
        params.append(createReadOnlyParam("Distribution type", "Uniform"));
    } else if (auto *s = dynamic_cast<StrategyPreventiveWithControl *>(strategy)) {
        auto *spin = createDoubleParam("Maintenance interval", 0.1, 100.0, 0.1, s->getMaintenanceInterval());
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [s](double val) { s->setMaintenanceInterval(val); });
        params.append(spin);
        params.append(createReadOnlyParam("Detect failure range", "0.5 – 1.5"));
        params.append(createReadOnlyParam("Recover range", "1.5 – 3.0"));
    } else if (auto *s = dynamic_cast<StrategyNoControlCheck *>(strategy)) {
        auto *p1 = createDoubleParam("Check period", 0.1, 100.0, 0.1, s->getCheckPeriod());
        auto *p2 = createDoubleParam("Check window", 0.01, 10.0, 0.01, s->getCheckWindow());

        connect(p1, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [s](double val) { s->setCheckPeriod(val); });
        connect(p2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [s](double val) { s->setCheckWindow(val); });

        params.append(p1);
        params.append(p2);
        params.append(createReadOnlyParam("Detect failure range", "2.0 – 4.0"));
        params.append(createReadOnlyParam("Recover range", "2.0 – 4.0"));
    } else if (auto *s = dynamic_cast<StrategyInstantCheckDetection *>(strategy)) {
        auto *p1 = createDoubleParam("Check period", 0.1, 100.0, 0.1, s->getCheckPeriod());
        auto *p2 = createDoubleParam("Check window", 0.01, 10.0, 0.01, s->getCheckWindow());

        connect(p1, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [s](double val) { s->setCheckPeriod(val); });
        connect(p2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [s](double val) { s->setCheckWindow(val); });

        params.append(p1);
        params.append(p2);
        params.append(createReadOnlyParam("Detect failure", "0.0 (instant at check)"));
        params.append(createReadOnlyParam("Recover range", "1.0 – 2.0"));
    } else if (auto *s = dynamic_cast<StrategyOfflineCheck *>(strategy)) {
        auto *p1 = createDoubleParam("Check period", 0.1, 100.0, 0.1, s->getCheckPeriod());
        auto *p2 = createDoubleParam("Check window", 0.01, 10.0, 0.01, s->getCheckWindow());

        connect(p1, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [s](double val) { s->setCheckPeriod(val); });
        connect(p2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [s](double val) { s->setCheckWindow(val); });

        params.append(p1);
        params.append(p2);
        params.append(createReadOnlyParam("Detect failure range", "1.0 – 2.5"));
        params.append(createReadOnlyParam("Recover range", "1.0 – 2.5"));
    } else if (auto *s = dynamic_cast<StrategyFixedIntervalControl *>(strategy)) {
        auto *spin = createDoubleParam("Maintenance interval", 0.1, 100.0, 0.1, s->getMaintenanceInterval());
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [s](double val) { s->setMaintenanceInterval(val); });
        params.append(spin);
        params.append(createReadOnlyParam("Detect failure range", "0.5 – 1.0"));
        params.append(createReadOnlyParam("Recover range", "1.0 – 2.0"));
    } else if (auto *s = dynamic_cast<StrategyPeriodicControlEmergencyRecovery *>(strategy)) {
        auto *spin = createDoubleParam("Control interval", 0.1, 100.0, 0.1, s->getControlInterval());
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [s](double val) { s->setControlInterval(val); });
        params.append(spin);
        params.append(createReadOnlyParam("Detect failure range", "0.0 – 0.5"));
        params.append(createReadOnlyParam("Recover range", "2.0 – 5.0"));
    }

    if (!params.isEmpty()) {
        strategyParamsWidgets[strategyName] = params;
        paramsContainer->show();
    } else {
        paramsContainer->hide();
    }
}
