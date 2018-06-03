#ifndef BITSTREAMLOADER_H
#define BITSTREAMLOADER_H

#include <QString>
#include <QThread>
#include "bitstream.h"

class BitstreamLoader : public QThread
{
    Q_OBJECT
public:
    BitstreamLoader(QObject *parent, QString filename);

private:
    QString _filename;

    void run() override;

signals:
    void progress(int cur, int max);
    void ready(Bitstream bitstream);
    void failed();
};

#endif // BITSTREAMLOADER_H
