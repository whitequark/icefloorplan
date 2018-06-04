#include <QApplication>
#include "bitstream.h"
#include "chipdb.h"
#include "floorplanwindow.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<ChipDB>();
    qRegisterMetaType<Bitstream>();

    QApplication a(argc, argv);
    FloorplanWindow w;
    w.show();

    return a.exec();
}
