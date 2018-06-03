#include <QtDebug>
#include <QOpenGLWidget>
#include <QWheelEvent>
#include <QGraphicsSimpleTextItem>
#include "floorplanwidget.h"
#include "chipdb.h"
#include "bitstream.h"

const float tileEdge   = 0.42;
const float wireSpc    = 0.0102;
const float span4Base  = -0.4;
const float span12Base = 0.4 - (12 * 2 - 1) * wireSpc;
const float spanShort  = -0.37;
const float spanShort2 = -0.282;
const float labelMax   = 0.4;

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
    : QGraphicsView(parent), _bitstream(nullptr), _chip(nullptr)
{
    setViewport(new QOpenGLWidget);
    setScene(&_scene);

    _scene.clear();
    _scene.setBackgroundBrush(Qt::white);
}

void FloorplanWidget::setData(Bitstream *bitstream, ChipDB *chip)
{
    _bitstream = bitstream;
    _chip = chip;
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
