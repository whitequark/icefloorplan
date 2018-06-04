#ifndef FLOORPLANWIDGET_H
#define FLOORPLANWIDGET_H

#include <QGestureEvent>
#include <QGraphicsPathItem>
#include <QGraphicsView>
#include "bitstream.h"
#include "chipdb.h"

class FloorplanWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit FloorplanWidget(QWidget *parent = nullptr);

    void setData(Bitstream *bitstream, ChipDB *chip);

public slots:
    void setUseOpenGL(bool on);

    void useVerboseLogicNotation();
    void useCompactLogicNotation();
    void useRawLogicNotation();

    void setShowUnusedLogic(bool on);

    void resetZoom();

signals:
    void netHovered(net_t net, QString name, QString symbol);

protected:
    bool viewportEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    virtual bool gestureEvent(QGestureEvent *event);

private:
    bool _useOpenGL;
    enum { VerboseLUTs, CompactLUTs, RawLUTs } _lutNotation;
    bool _showUnusedLogic;

    Bitstream *_bitstream;
    ChipDB *_chip;
    QGraphicsScene _scene;
    QGraphicsPathItem *_hovered;
    QPen _hoveredOldPen;

    bool _suppressDrag;

    void clear();
    void buildTiles();
    void buildTile(const Bitstream::Tile &tile);
    void buildLogicTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem);
    void buildIOTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem);
    void buildRAMTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem);

    QString recognizeFunction(uint lutData, bool hasA, bool hasB, bool hasC, bool hasD,
                              bool describeInputs = true) const;
};

#endif // FLOORPLANWIDGET_H
