#include <QtDebug>
#include <QApplication>
#include <QGraphicsPathItem>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QPinchGesture>
#include <QTouchEvent>
#include <QWheelEvent>
#include "floorplanwidget.h"
#include "bitstream.h"
#include "chipdb.h"

FloorplanWidget::FloorplanWidget(QWidget *parent)
    : QGraphicsView(parent), _useOpenGL(false), _lutNotation(FloorplanBuilder::VerboseLUTs),
      _showUnusedLogic(false), _bitstream(nullptr), _chipDB(nullptr), _hovered(nullptr)
{
    setUseOpenGL(_useOpenGL);
    setScene(&_scene);
    _scene.setBackgroundBrush(Qt::white);
}

void FloorplanWidget::setUseOpenGL(bool on)
{
    _useOpenGL = on;
    setViewport(_useOpenGL ? new QOpenGLWidget : new QWidget);
    viewport()->setMouseTracking(true);
    viewport()->grabGesture(Qt::PinchGesture);
}

void FloorplanWidget::useVerboseLogicNotation()
{
    _lutNotation = FloorplanBuilder::VerboseLUTs;
    rebuildTiles();
}

void FloorplanWidget::useCompactLogicNotation()
{
    _lutNotation = FloorplanBuilder::CompactLUTs;
    rebuildTiles();
}

void FloorplanWidget::useRawLogicNotation()
{
    _lutNotation = FloorplanBuilder::RawLUTs;
    rebuildTiles();
}

void FloorplanWidget::setShowUnusedLogic(bool on)
{
    _showUnusedLogic = on;
    rebuildTiles();
}

void FloorplanWidget::rebuildTiles()
{
    _hovered = nullptr;
    _scene.clear();
    FloorplanBuilder(_chipDB, _bitstream, &_scene, _lutNotation, _showUnusedLogic).buildTiles();
}

void FloorplanWidget::resetZoom()
{
    _scene.setSceneRect(_scene.itemsBoundingRect() + QMarginsF(100, 100, 100, 100));
    fitInView(_scene.sceneRect(), Qt::KeepAspectRatio);
}

void FloorplanWidget::setData(Bitstream *bitstream, ChipDB *chipDB)
{
    _bitstream = bitstream;
    _chipDB    = chipDB;

    rebuildTiles();
    resetZoom();
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
            _hovered = netItem;
        } else {
            _hovered = nullptr;
        }

        if(_hovered) {
            net_t net = netItem->data(0).toInt();
            QString symbol;
            if(_bitstream->symbols.contains(net)) {
                symbol = _bitstream->symbols[net];
            }
            emit netHovered(net, netItem->toolTip(), symbol);
        } else {
            emit netHovered(-1, QString(), QString());
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

bool FloorplanWidget::gestureEvent(QGestureEvent *event)
{
    if(auto pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture))) {
        if(pinch->changeFlags() & QPinchGesture::ScaleFactorChanged) {
            auto s = pinch->scaleFactor();
            scale(s, s);
            return true;
        }
    }

    return false;
}

bool FloorplanWidget::viewportEvent(QEvent *event)
{
    if(event->type() == QEvent::Gesture) {
        return gestureEvent(static_cast<QGestureEvent *>(event));
    } else {
        if(event->type() == QEvent::TouchEnd) {
            QTouchEvent *touch = static_cast<QTouchEvent *>(event);
            QPointF pos        = touch->touchPoints()[0].lastPos();
            QMouseEvent releaseEvent(QEvent::MouseButtonRelease, pos, Qt::LeftButton,
                                     Qt::LeftButton, touch->modifiers());
            QApplication::sendEvent(viewport(), &releaseEvent);
        }
        return QGraphicsView::viewportEvent(event);
    }
}
