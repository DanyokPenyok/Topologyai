#pragma once
#include <QException>
#include "edge.h"
#include "graphwidget.h"
#include "node.h"
#include "qmap.h"

typedef QList<QList<double>> Matrix2D;

class Graph {
public:
    Graph();
    Graph(unsigned int size);
    Graph(Matrix2D &matrix);
    ~Graph();
    const Matrix2D getMatrixAdjacent() const;
    const Matrix2D getMatrixFlow() const;
    const Matrix2D getMatrixBandwidth() const;
    const QList<QVariantList> getListEdges() const;
    void setMatrixAdjacent(Matrix2D &matrix);
    void setMatrixFlow(Matrix2D &matrix);
    void setMatrixBandwidth(Matrix2D &matrix);

    void setEdge(Node *u, Node *v, Edge &edge);

    void setEdgeFlow(unsigned int u, unsigned int v, double f);
    void setEdgeWeight(unsigned int u, unsigned int v, double w);
    void setEdgeBandwidth(unsigned int u, unsigned int v, double b);

    void removeEdge(unsigned int u, unsigned int v);
    void addNode(unsigned int i);

    const QFlags<GraphFlags> getFlags();
    void setFlag(GraphFlags flag);
    void setFlags(QFlags<GraphFlags> flags);
    void unsetFlag(GraphFlags flag);
    void toggleFlag(GraphFlags flag);

    //! @brief Получить доступ к карте рёбер (только для чтения)
    const QMap<QPair<Node *, Node *>, Edge *> &getEdges() const { return edges; }

    //! @brief Получить доступ к карте узлов (только для чтения)
    const QMap<unsigned int, Node *> &getNodes() const { return nodes; }

    GraphWidget *graphView;
    unsigned int getAmount() const;
    bool unsavedChanges = false;

    // --- ПЕРЕНЕСЕНО СЮДА ИЗ PRIVATE ---
    void resizeGraph(unsigned int oldAmount, unsigned int newAmount);

private:
    QFlags<GraphFlags> flags;
    EdgeType getEdgeType(int i, int j, Matrix2D &matrix);
    unsigned int amount;

    QMap<QPair<Node *, Node *>, Edge *> edges;
    QMap<unsigned int, Node *> nodes;

    Matrix2D adjacent;
    Matrix2D flow;
    Matrix2D bandwidth;
};
