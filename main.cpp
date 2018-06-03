#include <QApplication>
#include "floorplanwindow.h"
#include "chipdb.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<ChipDB>();

    QApplication a(argc, argv);
    FloorplanWindow w;
    w.show();

    return a.exec();
}
