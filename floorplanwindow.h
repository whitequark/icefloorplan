#ifndef FLOORPLANWINDOW_H
#define FLOORPLANWINDOW_H

#include <QThread>
#include <QMainWindow>
#include <QProgressBar>
#include "chipdb.h"

class ChipDBThread : public QThread
{
    Q_OBJECT
public:
    ChipDBThread(QObject *parent, QString name);

private:
    QString _name;

    void run() override;

signals:
    void progress(int cur, int max);
    void ready(ChipDB chipDB);
    void failed();
};

namespace Ui {
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
    ChipDB *_chipDB;

private slots:
    void openFile();
    void loadChipDB(QString name);
};

#endif // FLOORPLANWINDOW_H
