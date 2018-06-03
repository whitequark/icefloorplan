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

public slots:
    void setShowUnusedLogic(bool show);

    void resetZoom();

signals:
    void netHovered(net_t net, QString name, QString symbol);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool _showUnusedLogic;

    Bitstream *_bitstream;
    ChipDB *_chip;
    QGraphicsScene _scene;
    QGraphicsPathItem *_hovered;
    QPen _hoveredOldPen;

    void clear();
    void buildTiles();
    void buildTile(const Bitstream::Tile &tile);
    void buildLogicTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem);
};

#endif // FLOORPLANWIDGET_H
