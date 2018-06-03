#include <QtDebug>
#include <QOpenGLWidget>
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
    setViewport(new QOpenGLWidget);
    viewport()->setMouseTracking(true);
    setScene(&_scene);
    _scene.setBackgroundBrush(Qt::white);

    CircuitBuilder builder;

    builder.setColor(BLOCK_COLOR);
    // lutff_N (carry adder)
    builder.setOrigin(1, -1.5);
    builder.addMux(CircuitBuilder::Up, 1.5, 0, 3);
    QPointF carryI0 = builder.addPin(CircuitBuilder::Down, 0, 0);
    QPointF carryI1 = builder.addPin(CircuitBuilder::Down, 1, 0);
    QPointF carryI2 = builder.addPin(CircuitBuilder::Down, 2, 0);
    QPointF carryO  = builder.addPin(CircuitBuilder::Up,   1, 0);
    // lutff_N (look-up table)
    builder.setOrigin(5, 0);
    builder.addBlock(0, 0, 4, 4);
    QPointF lutI0 = builder.addPin(CircuitBuilder::Left,  0, 0, "A");
    QPointF lutI1 = builder.addPin(CircuitBuilder::Left,  0, 1, "B");
    QPointF lutI2 = builder.addPin(CircuitBuilder::Left,  0, 2, "C");
    QPointF lutI3 = builder.addPin(CircuitBuilder::Left,  0, 3, "D");
    QPointF lutO  = builder.addPin(CircuitBuilder::Right, 3, 0, "O");
    // lutff_N (flip-flop)
    builder.setOrigin(13, 0);
    builder.addBlock(0, 0, 3, 3);
    QPointF ffD   = builder.addPin(CircuitBuilder::Left,  0, 0, "D");
    QPointF ffEN  = builder.addPin(CircuitBuilder::Left,  0, 1, "EN");
    QPointF ffCLK = builder.addPin(CircuitBuilder::Left,  0, 2, ">");
    QPointF ffQ   = builder.addPin(CircuitBuilder::Right, 2, 0, "Q");
    QPointF ffRS  = builder.addPin(CircuitBuilder::Down,  1, 2, "RS");
    _scene.addItem(builder.build("lutff_0"));

    builder.setColor(NET_COLOR);
    builder.setOrigin(0, 0);
    // lutff_N/lout
    builder.moveTo(lutO);
    builder.segmentTo(ffD);
    _scene.addItem(builder.build("lutff_0/lout", 0));
    // lutff_N/in_0
    QPointF lutffI0 = builder.moveTo(0, 0);
    builder.segmentTo(lutI0);
    _scene.addItem(builder.build("lutff_0/in_0", 0));
    // lutff_N/in_1
    QPointF lutffI1 = builder.moveTo(0, 1);
    builder.segmentTo(lutI1);
    builder.junctionTo(carryI2.x(), lutI1.y());
    builder.segmentTo(carryI2);
    _scene.addItem(builder.build("lutff_0/in_1", 0));
    // lutff_N/in_2
    QPointF lutffI2 = builder.moveTo(0, 2);
    builder.segmentTo(lutI2);
    builder.junctionTo(carryI0.x(), lutI2.y());
    builder.segmentTo(carryI0);
    _scene.addItem(builder.build("lutff_0/in_2", 0));
    // lutff_N/in_3
    QPointF lutffI3 = builder.moveTo(0, 3);
    builder.segmentTo(lutI3);
    _scene.addItem(builder.build("lutff_0/in_3", 0));
    // carry_in
    QPointF plbCarryI = builder.moveTo(carryI1.x(), 4);
    builder.segmentTo(carryI1);
    _scene.addItem(builder.build("carry_in", 0));

    scale(4, 4);
}

void FloorplanWidget::setData(Bitstream *bitstream, ChipDB *chip)
{
    _bitstream = bitstream;
    _chip = chip;

    _scene.clear();
    makeTiles();

    resetZoom();
}

void FloorplanWidget::resetZoom()
{
    float s = qMin(width()  / (_chip->width + 1),
                   height() / (_chip->height + 1));
    setSceneRect(_scene.sceneRect() + QMarginsF(0.5, 0.5, 0.5, 0.5));
    resetTransform();
    scale(s, s);
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

void FloorplanWidget::makeTiles()
{
    for(auto &tile : _chip->tiles) {
        _scene.addRect(tile.x - tileEdge, tile.y - tileEdge,
                            2 * tileEdge,      2 * tileEdge,
                       Qt::NoPen, tileColor(tile.type, false));

        auto text = _scene.addSimpleText(QString("(%1 %2)").arg(tile.x).arg(tile.y));
        text->setPos(tile.x - tileEdge, tile.y - tileEdge);
        text->setTransformOriginPoint(0.005, -0.07);
        text->setScale(0.003);
    }
}
