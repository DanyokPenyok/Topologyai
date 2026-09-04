#include "node.h"
#include "edge.h"
#include "graphwidget.h"
#include "nodeinfowidget.h"

#include <QAction>
#include <QDockWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QStyleOption>

Node::Node(int index, GraphWidget *graphWidget) : graph(graphWidget) {
    this->index = index;
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setFlag(ItemIsSelectable);
    setAcceptHoverEvents(true);
    setCacheMode(DeviceCoordinateCache);
    setZValue(1);
    this->setPos(QRandomGenerator::global()->bounded(300), QRandomGenerator::global()->bounded(300));
}

Node::Node() : graph(nullptr), index(0) {}

void Node::addEdge(Edge *edge) {
    edgeList << edge;
    edge->adjust();
}

QSet<Edge *> Node::edges() const { return edgeList; }

void Node::calculateForces(bool manual) {
    if (!scene() || manual || scene()->mouseGrabberItem() == this) {
        newPos = pos();
        return;
    }

    qreal xvel = 0;
    qreal yvel = 0;
    const QList<QGraphicsItem *> items = scene()->items();
    for (QGraphicsItem *item: items) {
        Node *node = qgraphicsitem_cast<Node *>(item);
        if (!node) continue;

        QPointF vec = mapToItem(node, 0, 0);
        qreal dx = vec.x();
        qreal dy = vec.y();
        double l = 2.0 * (dx * dx + dy * dy);
        if (l > 0) {
            xvel += (dx * 550.0) / l;
            yvel += (dy * 550.0) / l;
        }
    }

    double weight = (edgeList.size() + 1) * 50;
    for (const Edge *edge: std::as_const(edgeList)) {
        QPointF vec;
        if (edge->sourceNode() == this)
            vec = mapToItem(edge->destNode(), 0, 0);
        else
            vec = mapToItem(edge->sourceNode(), 0, 0);
        xvel -= vec.x() / weight;
        yvel -= vec.y() / weight;
    }

    if (qAbs(xvel) < 0.1 && qAbs(yvel) < 0.1) xvel = yvel = 0;

    QRectF sceneRect = scene()->sceneRect();
    newPos = pos() + QPointF(xvel, yvel);
    newPos.setX(qMin(qMax(newPos.x(), sceneRect.left() + 10), sceneRect.right() - 10));
    newPos.setY(qMin(qMax(newPos.y(), sceneRect.top() + 10), sceneRect.bottom() - 10));
}

bool Node::advancePosition() {
    if (newPos == pos()) return false;
    setPos(newPos);
    return true;
}

void Node::connectToNode(Node *node) {
    if (node == nullptr) return;
    this->children.insert(node);
    node->parents.insert(this);
}

void Node::disconnectFromNode(Node *node) {
    if (node == nullptr) return;
    if (node == this) {
        if (children.find(this) != children.end()) {
            edgeList.removeIf([](Edge *i) { return i->getEdgeType() == EdgeType::Loop; });
            this->children.remove(this);
            this->parents.remove(this);
        }
    } else {
        if (children.find(node) != children.end()) {
            edgeList.removeIf([&node](Edge *i) { return i->destNode() == node; });
            node->edgeList.removeIf([this](Edge *i) { return i->sourceNode() == this; });
            this->children.remove(node);
            node->parents.remove(this);
        }
    }
}

QSet<Node *> Node::getChilden() { return this->children; }
QSet<Node *> Node::getParents() { return this->parents; }

QRectF Node::boundingRect() const {
    qreal adjust = 2;
    return QRectF(-nodeSize / 2 - adjust, -nodeSize / 2 - adjust, nodeSize + strokeWidth + 2 * adjust,
                  nodeSize + strokeWidth + 2 * adjust);
}

QPainterPath Node::shape() const {
    QPainterPath path;
    path.addEllipse(-nodeSize / 2, -nodeSize / 2, nodeSize, nodeSize);
    return path;
}

int Node::getIndex() { return this->index; }
void Node::setIndex(unsigned int num) { this->index = num; }
void Node::bindModel(NodeModel *model) {
    m_model = model;
    update();
}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) {
    QColor baseColor = QColor(210, 218, 228);
    if (m_model) {
        switch (m_model->state()) {
            case NodeModel::State::Operational:
                baseColor = QColor(76, 175, 80);
                break;
            case NodeModel::State::Failed:
                baseColor = QColor(244, 67, 54);
                break;
            case NodeModel::State::Maintenance:
                baseColor = QColor(255, 193, 7);
                break;
        }
    }

    painter->setRenderHint(QPainter::Antialiasing, true);

    QRadialGradient fillGradient(QPointF(-8, -10), nodeSize);
    fillGradient.setColorAt(0.0, baseColor.lighter(165));
    fillGradient.setColorAt(1.0, baseColor.darker(120));
    painter->setBrush(fillGradient);

    const bool isActive = isSelected() || (option->state & QStyle::State_MouseOver);
    painter->setPen(QPen(isActive ? QColor(33, 150, 243) : QColor(45, 55, 72), isActive ? 3 : 2));
    painter->drawPath(shape());

    if (m_model) {
        const qreal ringSpan = qBound(0.0, m_model->reliability(), 1.0) * 360.0;
        QRectF ringRect(-nodeSize / 2.0 + 4, -nodeSize / 2.0 + 4, nodeSize - 8, nodeSize - 8);
        painter->setPen(QPen(QColor(255, 255, 255, 210), 3, Qt::SolidLine, Qt::RoundCap));
        painter->drawArc(ringRect, 90 * 16, -static_cast<int>(ringSpan * 16.0));
    }

    QFont font = painter->font();
    font.setBold(true);
    font.setPointSize(12);
    painter->setFont(font);

    QString nodeText = QString::number(index);
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(nodeText);

    QPointF textPnt(-(textRect.width()) / 2 - 0.7, textRect.height() / 3. - 0.7);
    painter->setPen(QColor(20, 20, 20));
    painter->drawText(textPnt, nodeText);
}

Node::~Node() {
    for (auto &child: std::as_const(children)) this->disconnectFromNode(child);
    for (auto &parent: std::as_const(parents)) parent->disconnectFromNode(this);
}

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged) {
        for (Edge *edge: std::as_const(edgeList)) edge->adjust();
        if (graph) graph->runTimer();
    }
    return QGraphicsItem::itemChange(change, value);
}

void Node::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (graph) {
        QWidget *mainWindow = graph->window();
        if (mainWindow) {
            if (QDockWidget *dock = mainWindow->findChild<QDockWidget*>("dock_NodeInfo")) {
                dock->show();
                dock->raise();

                if (QAction *dockAction = dock->toggleViewAction()) {
                    dockAction->setChecked(true);
                }
            }
            if (NodeInfoWidget *sidebar = mainWindow->findChild<NodeInfoWidget*>()) {
                sidebar->setNode(this);
            }
        }
    }
    update();
    QGraphicsItem::mousePressEvent(event);
}

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    update();
    QGraphicsItem::mouseReleaseEvent(event);
}
