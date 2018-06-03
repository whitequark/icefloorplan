#ifndef CIRCUITBUILDER_H
#define CIRCUITBUILDER_H

#include <QPainterPath>
#include <QPen>
#include <QFont>
#include "chipdb.h"

class QGraphicsPathItem;

class CircuitBuilder
{
public:
    enum Direction {
        Up, Right, Down, Left
    };

    CircuitBuilder();

    void setGrid(qreal grid);
    void setColor(const QColor &color);

    void setOrigin(qreal x, qreal y);

    QPointF moveTo(qreal x, qreal y);
    QPointF moveTo(QPointF p);
    QPointF segmentTo(qreal x, qreal y);
    QPointF segmentTo(QPointF p);
    QPointF junctionTo(qreal x, qreal y);
    QPointF junctionTo(QPointF p);

    void addBlock(qreal x, qreal y, qreal w, qreal h);
    void addMux(Direction dir, qreal x, qreal y, qreal w, qreal h = 1);

    QPointF addPin(Direction dir, qreal x, qreal y, const QString &name = "");
    void addClockSymbol(Direction dir, qreal x, qreal y);
    void addLabel(Direction anchor, qreal x, qreal y, const QString &text);

    QGraphicsPathItem *build(const QString &toolTip = "", net_t net = -1);

private:
    qreal _grid;
    QPointF _origin;
    QPen _pen;
    QFont _font;
    QPainterPath _path, _textPath;

    static void directionToVectors(Direction dir, QPointF *h, QPointF *v);
};

#endif // CIRCUITBUILDER_H
