#include <QtDebug>
#include <QGraphicsPathItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include "chipdb.h"
#include "bitstream.h"
#include "circuitbuilder.h"
#include "floorplanwidget.h"

const float tileEdge   = 0.42;

static const QColor BLOCK_COLOR = Qt::darkRed;
static const QColor NET_COLOR   = Qt::darkGreen;

static QColor tileColor(const QString &type, bool active)
{
    if(type == "io") {
        return active ? QColor::fromRgb(0xAAEEEE) : QColor::fromRgb(0x88AAAA);
    } else if(type == "logic") {
        return active ? QColor::fromRgb(0xEEAAEE) : QColor::fromRgb(0xA383A3);
    } else if(type == "ramb" || type == "ramt") {
        return active ? QColor::fromRgb(0xEEEEAA) : QColor::fromRgb(0xAAAA88);
    } else {
        return QColor::fromRgb(0x000000);
    }
}

FloorplanWidget::FloorplanWidget(QWidget *parent)
    : QGraphicsView(parent), _bitstream(nullptr), _chip(nullptr), _hovered(nullptr)
{
    viewport()->setMouseTracking(true);

    setScene(&_scene);
    _scene.setBackgroundBrush(Qt::white);
}

void FloorplanWidget::resetZoom()
{
    float s = qMin(width()  / (_chip->width + 1),
                   height() / (_chip->height + 1));
    setSceneRect(_scene.sceneRect() + QMarginsF(50, 50, 50, 50));
    resetTransform();
    fitInView(sceneRect(), Qt::KeepAspectRatio);
//    scale(s, s);
}

void FloorplanWidget::wheelEvent(QWheelEvent *event)
{
    if(event->modifiers() == Qt::ControlModifier) {
        float s = 1 + event->angleDelta().y() / 1200.0;
        scale(s, s);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void FloorplanWidget::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsPathItem *netItem = nullptr;

    QRect hoverRect = QRect(event->pos(), QSize()).adjusted(-10, -10, 10, 10);
    for(QGraphicsItem *hoverItem : items(hoverRect)) {
        QGraphicsPathItem *item = qgraphicsitem_cast<QGraphicsPathItem *>(hoverItem);
        if(item->data(0).isValid()) {
            netItem = item;
            break;
        }
    }

    if(netItem != _hovered) {
        if(_hovered) {
            _hovered->setPen(_hoveredOldPen);
        }
        if(netItem) {
            _hoveredOldPen = netItem->pen();
            netItem->setPen(QPen(Qt::red));
            emit netHovered(netItem->data(0).toInt(), netItem->toolTip());
            _hovered = netItem;
        } else {
            emit netHovered(-1, "");
            _hovered = nullptr;
        }
    }
}

void FloorplanWidget::setData(Bitstream *bitstream, ChipDB *chip)
{
    _bitstream = bitstream;
    _chip = chip;

    _scene.clear();
    buildTile(5, 5);

    resetZoom();
}

void FloorplanWidget::buildTile(coord_t x, coord_t y)
{
    const QString &type = _chip->tile(x, y).type;
    if(type == "logic") {
        buildLogicTile(x, y);
    }
}

void FloorplanWidget::buildLogicTile(coord_t x, coord_t y)
{
    CircuitBuilder builder;

    QVector<QPointF> ffENs, ffCLKs, ffRSs;
    for(int lc = 7; lc >= 0; lc--) {
        qreal lcOff = lc * 6;

        builder.setColor(BLOCK_COLOR);
        // lutff_N (carry adder)
        builder.setOrigin(1, lcOff - 1.5);
        builder.addMux(CircuitBuilder::Up, 1.5, 0, 3);
        builder.addLabel(CircuitBuilder::Up, 1, 0, "+");
        QPointF carryI0 = builder.addPin(CircuitBuilder::Down, 0, 0);
        QPointF carryI1 = builder.addPin(CircuitBuilder::Down, 1, 0);
        QPointF carryI2 = builder.addPin(CircuitBuilder::Down, 2, 0);
        QPointF carryO  = builder.addPin(CircuitBuilder::Up,   1, 0);
        // lutff_N (look-up table)
        builder.setOrigin(5, lcOff);
        builder.addBlock(0, 0, 4, 4);
        QPointF lutI0 = builder.addPin(CircuitBuilder::Left,  0, 0, "A");
        QPointF lutI1 = builder.addPin(CircuitBuilder::Left,  0, 1, "B");
        QPointF lutI2 = builder.addPin(CircuitBuilder::Left,  0, 2, "C");
        QPointF lutI3 = builder.addPin(CircuitBuilder::Left,  0, 3, "D");
        QPointF lutO  = builder.addPin(CircuitBuilder::Right, 3, 0, "O");
        // lutff_N (flip-flop)
        builder.setOrigin(14, lcOff);
        builder.addBlock(0, 0, 3, 3);
        QPointF ffD   = builder.addPin(CircuitBuilder::Left,  0, 0, "D");
        QPointF ffEN  = builder.addPin(CircuitBuilder::Left,  0, 1, "EN");
        QPointF ffCLK = builder.addPin(CircuitBuilder::Left,  0, 2, ">");
        QPointF ffQ   = builder.addPin(CircuitBuilder::Right, 2, 0, "Q");
        QPointF ffRS  = builder.addPin(CircuitBuilder::Down,  1, 2, "RS");
        _scene.addItem(builder.build(QString("lutff_%1").arg(lc)));

        builder.setColor(NET_COLOR);
        builder.setOrigin(0, lcOff);
        // lutff_N/lout
        builder.moveTo(lutO);
        builder.segmentTo(ffD);
        _scene.addItem(builder.build(QString("lutff_%1/lout").arg(lc), 0));
        // lutff_N/in_0
        QPointF lutffI0 = builder.moveTo(0, 0);
        builder.segmentTo(lutI0);
        _scene.addItem(builder.build(QString("lutff_%1/in_0").arg(lc), 0));
        // lutff_N/in_1
        QPointF lutffI1 = builder.moveTo(0, 1);
        builder.segmentTo(lutI1);
        builder.junctionTo(QPointF(carryI2.x(), lutI1.y()));
        builder.segmentTo(carryI2);
        _scene.addItem(builder.build(QString("lutff_%1/in_1").arg(lc), 0));
        // lutff_N/in_2
        QPointF lutffI2 = builder.moveTo(0, 2);
        builder.segmentTo(lutI2);
        builder.junctionTo(QPointF(carryI0.x(), lutI2.y()));
        builder.segmentTo(carryI0);
        _scene.addItem(builder.build(QString("lutff_%1/in_2").arg(lc), 0));
        // lutff_N/in_3
        QPointF lutffI3 = builder.moveTo(0, 3);
        builder.segmentTo(lutI3);
        _scene.addItem(builder.build(QString("lutff_%1/in_3").arg(lc), 0));
        // carry_in
        QPointF tileCarryI = builder.moveTo(carryI1.x(), 3.5);
        builder.segmentTo(carryI1);
        _scene.addItem(builder.build("carry_in", 0));

        ffENs.append(ffEN);
        ffCLKs.append(ffCLK);
        ffRSs.append(ffRS);
    }

    builder.setOrigin(0, -3);
    QPointF tileRS = builder.moveTo(10, 0);
    builder.segmentTo(10, 8 * 6 + 1);
    for(QPointF lcRS : ffRSs) {
        builder.junctionTo(QPointF(tileRS.x(), lcRS.y()));
        builder.segmentTo(lcRS);
    }
    _scene.addItem(builder.build("lutff_global/s_r", 0));

    builder.setOrigin(0, -3);
    QPointF tileEN = builder.moveTo(11, 0);
    builder.segmentTo(11, 8 * 6 + 1);
    for(QPointF lcEN : ffENs) {
        builder.junctionTo(QPointF(tileEN.x(), lcEN.y()));
        builder.segmentTo(lcEN);
    }
    _scene.addItem(builder.build("lutff_global/cen", 0));

    builder.setOrigin(0, -3);
    QPointF tileCLK = builder.moveTo(12, 0);
    builder.segmentTo(12, 8 * 6 + 1);
    for(QPointF lcCLK : ffCLKs) {
        builder.junctionTo(QPointF(tileCLK.x(), lcCLK.y()));
        builder.segmentTo(lcCLK);
    }
    _scene.addItem(builder.build("lutff_global/clk", 0));
}

//void FloorplanWidget::makeTiles()
//{
//    for(auto &tile : _chip->tiles) {
//        _scene.addRect(tile.x - tileEdge, tile.y - tileEdge,
//                            2 * tileEdge,      2 * tileEdge,
//                       Qt::NoPen, tileColor(tile.type, false));

//        auto text = _scene.addSimpleText(QString("(%1 %2)").arg(tile.x).arg(tile.y));
//        text->setPos(tile.x - tileEdge, tile.y - tileEdge);
//        text->setTransformOriginPoint(0.005, -0.07);
//        text->setScale(0.003);
//    }
//}
