#include <QFile>
#include "bitstreamloader.h"

BitstreamLoader::BitstreamLoader(QObject *parent, QString filename)
    : QThread(parent), _filename(filename)
{}

void BitstreamLoader::run() {
    QFile file(_filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    Bitstream bitstream;
    if(bitstream.parse(&file, [=](int cur, int max) {
        emit progress(cur, max);
    })) {
        emit ready(bitstream);
    } else {
        emit failed();
    }
}
