#ifndef CIRCUITBUILDER_H
#define CIRCUITBUILDER_H

#include <QFont>
#include <QPainterPath>
#include <QPen>
#include "chipdb.h"

class QGraphicsItem;
class QGraphicsPathItem;

class CircuitBuilder
{
public:
    enum Direction { Up, Right, Down, Left };

    CircuitBuilder(QGraphicsItem *parent);

    void setGrid(qreal grid);
    void setColor(const QColor &color);

    void setOrigin(qreal x, qreal y);

    QPointF moveTo(qreal x, qreal y);
    QPointF moveTo(QPointF p);
    QPointF segmentTo(qreal x, qreal y);
    QPointF segmentTo(QPointF p);
    QPointF junctionAt(qreal x, qreal y);
    QPointF junctionAt(QPointF p);
    QPointF junctionTo(qreal x, qreal y);
    QPointF junctionTo(QPointF p);
    void junction(bool condition = true);
    QPointF joinTo(qreal x, qreal y, bool junction);
    QPointF joinTo(QPointF p, bool junction);

    void addBlock(qreal x, qreal y, qreal w, qreal h);
    void addMux(Direction dir, qreal x, qreal y, qreal w, qreal h = 1);
    void addBuffer(Direction dir, qreal x, qreal y);

    QPointF addPin(Direction dir, qreal x, qreal y, const QString &name = "",
                   bool activeLow = false);
    void addClockSymbol(Direction dir, qreal x, qreal y);
    void addLabel(Direction anchor, qreal x, qreal y, const QString &text);

    void addText(qreal x, qreal y, QString text, qreal size = 1);

    QGraphicsPathItem *build(const QString &toolTip = "", net_t net = -1);

private:
    qreal _grid;
    QPointF _origin;
    QPen _pen;
    QFont _font;
    QGraphicsItem *_parent;
    QPainterPath _path, _textPath;

    static void directionToVectors(Direction dir, QPointF *h, QPointF *v);
};

#endif // CIRCUITBUILDER_H
