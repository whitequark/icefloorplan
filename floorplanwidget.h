#ifndef FLOORPLANWIDGET_H
#define FLOORPLANWIDGET_H

#include <QGraphicsView>
#include <QGraphicsPathItem>
#include "chipdb.h"
#include "bitstream.h"

class FloorplanWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit FloorplanWidget(QWidget *parent = nullptr);

    void setData(Bitstream *bitstream, ChipDB *chip);
    void resetZoom();

signals:
    void netHovered(net_t net, QString name);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    Bitstream *_bitstream;
    ChipDB *_chip;
    QGraphicsScene _scene;
    QGraphicsPathItem *_hovered;
    QPen _hoveredOldPen;

    void makeTiles();
};

#endif // FLOORPLANWIDGET_H
