#include <QFile>
#include "chipdbloader.h"

ChipDBLoader::ChipDBLoader(QObject *parent, QString device) : QThread(parent), _device(device)
{}

void ChipDBLoader::run()
{
    QFile file(":/chipdb/" + _device + ".txt");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    ChipDB db;
    db.name = _device;
    if(db.parse(&file, [=](int cur, int max) { emit progress(cur, max); })) {
        emit ready(db);
    } else {
        emit failed();
    }
}
