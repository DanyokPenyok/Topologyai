#pragma once

#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <qspinbox.h>

class Node;
class QButtonGroup;
class IMaintenanceStrategy;

class NodeInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit NodeInfoWidget(QWidget *parent = nullptr);
    void setNode(Node *node);

private slots:
    void onStrategyToggled(bool checked);
    void refreshView();
    void onDistributionTypeChanged(int index);
    void onDistributionParamsChanged();

private:
    Node *currentNode = nullptr;
    QLabel *idValue = nullptr;
    QLabel *stateValue = nullptr;
    QLabel *reliabilityValue = nullptr;
    QLabel *deliveryValue = nullptr;
    QLabel *summaryValue = nullptr;
    QPlainTextEdit *edgeListValue = nullptr;
    QPlainTextEdit *inboxValue = nullptr;

    // === Выбор стратегии (8 чекбоксов) ===
    QCheckBox *strategyBasicControlCheck = nullptr;
    QCheckBox *strategyInstantDetectionCheck = nullptr;
    QCheckBox *strategyPreventiveWithControlCheck = nullptr;
    QCheckBox *strategyNoControlCheckCheck = nullptr;
    QCheckBox *strategyInstantCheckDetectionCheck = nullptr;
    QCheckBox *strategyOfflineCheckCheck = nullptr;
    QCheckBox *strategyFixedIntervalControlCheck = nullptr;
    QCheckBox *strategyPeriodicControlEmergencyRecoveryCheck = nullptr;

    QButtonGroup *strategyGroup = nullptr;

    // === Поля параметров стратегий ===
    QWidget *paramsContainer = nullptr;
    QVBoxLayout *paramsLayout = nullptr;
    QMap<QString, QList<QWidget*>> strategyParamsWidgets;

    QTimer *refreshTimer = nullptr;
    bool guardStrategyUpdate = false;

    void clearView();
    void syncStrategyChecks();
    void updateParamsVisibility();
    std::unique_ptr<IMaintenanceStrategy> createStrategyFor(QObject *checkBox) const;

    QDoubleSpinBox* createDoubleParam(const QString& label, double min, double max, double step, double value);
    QLabel* createReadOnlyParam(const QString& label, const QString& value);

    QGroupBox* m_distGroupBox = nullptr;
    QComboBox* m_distTypeCombo = nullptr;
    QDoubleSpinBox* m_expRateSpin = nullptr;
    QDoubleSpinBox* m_normMeanSpin = nullptr;
    QDoubleSpinBox* m_normStdSpin = nullptr;
    bool guardDistUpdate = false;
};
