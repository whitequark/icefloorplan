#ifndef FLOORPLANWIDGET_H
#define FLOORPLANWIDGET_H

#include <QGestureEvent>
#include <QGraphicsPathItem>
#include <QGraphicsView>
#include "bitstream.h"
#include "chipdb.h"
#include "floorplanbuilder.h"

class FloorplanWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit FloorplanWidget(QWidget *parent = nullptr);

    void setData(Bitstream *bitstream, ChipDB *chipDB);

public slots:
    void setUseOpenGL(bool on);

    void useVerboseLogicNotation();
    void useCompactLogicNotation();
    void useRawLogicNotation();

    void setShowUnusedLogic(bool on);

    void rebuildTiles();
    void resetZoom();

signals:
    void netHovered(net_t net, QString name, QString symbol);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    virtual bool gestureEvent(QGestureEvent *event);

private:
    bool _useOpenGL;
    FloorplanBuilder::LUTNotation _lutNotation;
    bool _showUnusedLogic;

    Bitstream *_bitstream;
    ChipDB *_chipDB;
    QGraphicsScene _scene;
    QGraphicsPathItem *_hovered;
    QPen _hoveredOldPen;

    bool _suppressDrag;
};

#endif // FLOORPLANWIDGET_H
