#ifndef FLOORPLANWINDOW_H
#define FLOORPLANWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include "bitstream.h"
#include "chipdb.h"

namespace Ui
{
class FloorplanWindow;
}

class FloorplanWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit FloorplanWindow(QWidget *parent = 0);
    ~FloorplanWindow();

private:
    Ui::FloorplanWindow *_ui;
    QProgressBar _progressBar;

    QMap<QString, ChipDB> _chipDBCache;
    Bitstream _bitstream;

private slots:
    void openExample();
    void openFile();
    void loadBitstream(QString filename);
    void loadChipDB(QString device);

private:
    void updateFloorplan();
};

#endif // FLOORPLANWINDOW_H
