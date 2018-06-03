#include <QApplication>
#include "floorplanwindow.h"
#include "chipdb.h"
#include "bitstream.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<ChipDB>();
    qRegisterMetaType<Bitstream>();

    QApplication a(argc, argv);
    FloorplanWindow w;
    w.show();

    return a.exec();
}
