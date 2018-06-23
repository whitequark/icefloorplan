#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <functional>

#include <QBitArray>
#include <QIODevice>
#include <QMap>
#include <QSet>
#include <QString>
#include "chipdb.h"

class Bitstream
{
public:
    struct Tile {
        coord_t x;
        coord_t y;
        QString type;
        QBitArray bits;

        uint extract(const QVector<nbit_t> &nbits) const;
    };

    Bitstream();
    bool parse(QIODevice *in, std::function<void(int, int)> progress);
    bool process(ChipDB &chip);

    Tile &tile(coord_t x, coord_t y);

    QString comment;
    QString device;
    QMap<QPair<coord_t, coord_t>, Tile> tiles;
    QMap<net_t, QString> symbols;

    QMap<QPair<Tile *, QString>, net_t> tileNets;
    QVector<net_t> netDrivers;
    QBitArray netLoaded;
};

Q_DECLARE_METATYPE(Bitstream)

#endif // BITSTREAM_H
