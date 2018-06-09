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
    void setOrigin(qreal x, qreal y);
    void setColor(const QColor &color);

    /// Set current position to `origin+(x,y)`.
    /// Return new current position.
    QPointF moveTo(qreal x, qreal y);
    /// Set current position to `p`.
    /// Return new current position.
    QPointF moveTo(QPointF p);

    /// Draw a wire from current position to new current position `origin+(x,y)`.
    /// Return new current position.
    QPointF wireTo(qreal x, qreal y);
    /// Draw a wire from current position to new current position `p`.
    /// Return new current position.
    QPointF wireTo(QPointF p);

    /// Draw a junction at position `origin+(x,y)` without changing current position.
    /// Return `origin+(x,y)`.
    QPointF junctionAt(qreal x, qreal y);
    /// Draw a junction at position `p` without changing current positoin.
    /// Return `p`.
    QPointF junctionAt(QPointF p);

    /// Draw a wire from current position to new current position `origin+(x,y)`
    /// and draw a junction at new current position.
    /// Return new current position.
    QPointF junctionTo(qreal x, qreal y);
    /// Draw a wire from current position to new current position `p`
    /// and draw a junction at new current position.
    /// Return new current position.
    QPointF junctionTo(QPointF p);

    /// Draw a junction at current position if `condition` is true.
    void junction(bool condition = true);

    /// Draw a wire from current position to new current position `origin+(x,y)`
    /// and draw a junction at new current position if `junction` is true.
    /// Return new current position.
    QPointF joinTo(qreal x, qreal y, bool junction);
    /// Draw a wire from current position to new current position `p`
    /// and draw a junction at new current position if `junction` is true.
    /// Return new current position.
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
