#include <QGraphicsPathItem>
#include <QFont>
#include <QFontMetrics>
#include "circuitbuilder.h"

CircuitBuilder::CircuitBuilder()
    : _pen(Qt::black), _font("fixed")
{
    _pen.setCapStyle(Qt::FlatCap);
    setGrid(20);
}

void CircuitBuilder::setGrid(qreal grid)
{
    _grid = grid;
    _font.setPixelSize(grid * 0.7);
}

void CircuitBuilder::setColor(const QColor &color)
{
    _pen = QPen(color);
}

void CircuitBuilder::setOrigin(qreal x, qreal y)
{
    _origin = QPointF(x, y);
}

QGraphicsPathItem *CircuitBuilder::build(const QString &toolTip, net_t net)
{
    QGraphicsPathItem *item = new QGraphicsPathItem;
    item->setPath(_path);
    item->setPen(_pen);
    item->setToolTip(toolTip);
    if(net != -1) {
        item->setData(0, net);
    }

    QGraphicsPathItem *textItem = new QGraphicsPathItem(item);
    textItem->setPath(_textPath);
    textItem->setPen(Qt::NoPen);
    textItem->setBrush(_pen.brush());

    _path = QPainterPath();
    _textPath = QPainterPath();
    return item;
}

QPointF CircuitBuilder::moveTo(qreal x, qreal y)
{
    QPointF p = _origin + QPointF(x, y);
    return moveTo(p);
}

QPointF CircuitBuilder::moveTo(QPointF p)
{
    _path.moveTo((p + QPointF(0.5, 0.5)) * _grid);
    return p;
}

QPointF CircuitBuilder::segmentTo(qreal x, qreal y)
{
    QPointF p = _origin + QPointF(x, y);
    return segmentTo(p);
}

QPointF CircuitBuilder::segmentTo(QPointF p)
{
    _path.lineTo((p + QPointF(0.5, 0.5)) * _grid);
    return p;
}

QPointF CircuitBuilder::junctionTo(qreal x, qreal y)
{
    QPointF p = _origin + QPointF(x, y);
    return junctionTo(p);
}

QPointF CircuitBuilder::junctionTo(QPointF p)
{
    _path.addEllipse((p + QPointF(0.5, 0.5)) * _grid, _pen.widthF(), _pen.widthF());
    moveTo(p);
    return p;
}

void CircuitBuilder::addBlock(qreal x, qreal y, qreal w, qreal h)
{
    _path.addRect(QRectF((_origin + QPointF(x, y)) * _grid,
                         QSizeF (w * _grid, h * _grid)));
}

void CircuitBuilder::directionToVectors(CircuitBuilder::Direction dir,
                                        QPointF *h, QPointF *v)
{
    switch(dir) {
    case Up:    *h = QPointF(1, 0); *v = QPointF(0, -1); break;
    case Down:  *h = QPointF(1, 0); *v = QPointF(0,  1); break;
    case Left:  *h = QPointF(0, 1); *v = QPointF(-1, 0); break;
    case Right: *h = QPointF(0, 1); *v = QPointF( 1, 0); break;
    }
}

void CircuitBuilder::addMux(CircuitBuilder::Direction dir,
                            qreal x, qreal y, qreal width, qreal height)
{
    QPointF h, v;
    directionToVectors(dir, &h, &v);

    QPainterPath muxPath;
    muxPath.lineTo((h * width) * _grid);
    muxPath.lineTo((h * (width - height) + v * height) * _grid);
    muxPath.lineTo((h * height + v * height) * _grid);
    muxPath.lineTo(QPointF());
    muxPath.translate(-(h * width / 2 + v * height) * _grid);

    muxPath.translate((_origin + QPointF(x, y)) * _grid);
    _path.addPath(muxPath);
}

QPointF CircuitBuilder::addPin(CircuitBuilder::Direction dir, qreal x, qreal y,
                               const QString &name)
{
    QPointF h, v;
    directionToVectors(dir, &h, &v);

    QPainterPath pinPath;
    pinPath.moveTo(0.5 * v * _grid);
    pinPath.lineTo(v * _grid);

    pinPath.translate((_origin + QPointF(x + 0.5, y + 0.5)) * _grid);
    _path.addPath(pinPath);

    if(name == ">") {
        addClockSymbol(dir, x, y);
    } else if(name != "") {
        addLabel(dir, x, y, name);
    }

    return _origin + QPointF(x, y) + v;
}

void CircuitBuilder::addClockSymbol(CircuitBuilder::Direction dir, qreal x, qreal y)
{
    QPointF h, v;
    directionToVectors(dir, &h, &v);

    QPainterPath symbolPath;
    symbolPath.moveTo((v * 0.5 - h * 0.4) * _grid);
    symbolPath.lineTo( v * 0.1      * _grid);
    symbolPath.lineTo((v * 0.5 + h * 0.4) * _grid);

    symbolPath.translate((_origin + QPointF(x + 0.5, y + 0.5)) * _grid);
    _path.addPath(symbolPath);
}

void CircuitBuilder::addLabel(CircuitBuilder::Direction anchor, qreal x, qreal y,
                              const QString &text)
{
    QFontMetrics metrics(_font);
    QRect bounds = metrics.boundingRect(text);

    QPainterPath labelPath;
    labelPath.addText(-bounds.width() / 2, metrics.ascent() / 2, _font, text);
    if(anchor == Right) {
        labelPath.translate((x + 0.8) * _grid - bounds.width() / 2, (y + 0.45) * _grid);
    } else if(anchor == Left) {
        labelPath.translate((x + 0.2) * _grid + bounds.width() / 2, (y + 0.45) * _grid);
    } else {
        labelPath.translate((x + 0.5) * _grid, (y + 0.45) * _grid);
    }
    _textPath.addPath(labelPath.translated(_origin * _grid));
}
