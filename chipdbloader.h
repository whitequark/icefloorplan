#ifndef CHIPDBLOADER_H
#define CHIPDBLOADER_H

#include <QThread>
#include <QString>
#include "chipdb.h"

class ChipDBLoader : public QThread
{
    Q_OBJECT
public:
    ChipDBLoader(QObject *parent, QString name);

private:
    QString _device;

    void run() override;

signals:
    void progress(int cur, int max);
    void ready(ChipDB chipDB);
    void failed();
};

#endif // CHIPDBLOADER_H
