#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <QString>
#include <QBitArray>
#include <QMap>
#include <QIODevice>
#include "chipdb.h"

class Bitstream
{
public:
    struct Tile {
        coord_t x;
        coord_t y;
        QString type;
        QBitArray bits;
    };

    Bitstream();
    bool parse(QIODevice *in, std::function<void(int,int)> progress);
    bool validate(ChipDB &chip);

    Tile &tile(coord_t x, coord_t y);

    QString comment;
    QString device;
    QMap<QPair<coord_t, coord_t>, Tile> tiles;
    QMap<net_t, QString> symbols;
};

Q_DECLARE_METATYPE(Bitstream)

#endif // BITSTREAM_H
