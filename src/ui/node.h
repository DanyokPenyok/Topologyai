#pragma once

#include <QGraphicsItem>
#include <QList>
#include <QSet>
#include <QString>
#include "nodemodel.h"

#define nodeSize 44
#define strokeWidth 1

class Edge;
class GraphWidget;
class Node;

// --- Класс-контейнер с информацией об узле ---
class NodeData {
public:
    NodeData() : test1(""), test2(""), test3("") {}

    QString test1;
    QString test2;
    QString test3;
};

// --- Класс узла графа ---
class Node : public QGraphicsItem {
public:
    Node(int index, GraphWidget *graphWidget);
    Node();
    Node(const Node &node);
    void addEdge(Edge *edge);
    QSet<Edge *> edges() const;

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    void calculateForces(bool manual);
    bool advancePosition();

    void connectToNode(Node *node);
    void disconnectFromNode(Node *node);
    QSet<Node *> getChilden();
    QSet<Node *> getParents();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    int getIndex();
    void setIndex(unsigned int);

    //! @brief Привязать чистую модель к графическому элементу
    void bindModel(NodeModel *model);

    //! @brief Получить привязанную модель
    NodeModel *model() const { return m_model; }

    Node &operator=(const Node &);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    ~Node();

    QSet<Edge *> edgeList;
    NodeData nodeData;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QSet<Node *> children;
    QSet<Node *> parents;

    QPointF newPos;
    GraphWidget *graph;
    int index;

    NodeModel *m_model = nullptr;
};
