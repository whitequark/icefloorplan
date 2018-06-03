#ifndef FLOORPLANWIDGET_H
#define FLOORPLANWIDGET_H

#include <QGraphicsView>

class ChipDB;

class FloorplanWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit FloorplanWidget(QWidget *parent = nullptr);

    void setChip(ChipDB *chip);
    void resetZoom();

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    ChipDB *_chip;
    QGraphicsScene _scene;

    void makeTiles();
};

#endif // FLOORPLANWIDGET_H
