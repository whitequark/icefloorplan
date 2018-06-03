#ifndef CHIPDB_H
#define CHIPDB_H

#include <QVector>
#include <QMap>
#include <QIODevice>

typedef uint8_t  coord_t;
typedef uint16_t nbit_t;
typedef int32_t  net_t;

class ChipDB
{
public:
    struct Pin {
        QString name;
        coord_t tile_x;
        coord_t tile_y;
        net_t net;
    };

    struct Package {
        QString name;
        QMap<QString, Pin> pins;
    };

    struct TileBits {
        QString type;
        nbit_t columns;
        nbit_t rows;
        QMap<QString, QVector<nbit_t>> functions;
    };

    struct Connection {
        net_t dst_net_num;
        QVector<net_t> src_net_nums;
        QVector<nbit_t> bits;
    };

    struct Tile {
        coord_t x;
        coord_t y;
        QString type;
        QVector<Connection> buffers;
        QVector<Connection> routing;
    };

    struct TileNet {
        coord_t tile_x;
        coord_t tile_y;
        QString name;
    };

    struct Net {
        net_t num;
        QString kind;
        QVector<TileNet> entries;
    };

    ChipDB();
    bool parse(QIODevice *in, std::function<void(int,int)> progress);

    Tile &tile(coord_t x, coord_t y);

    QString name;
    coord_t width;
    coord_t height;
    QMap<QString, Package> packages;
    QMap<QString, TileBits> tilesBits;
    QMap<QPair<coord_t, coord_t>, Tile> tiles;
    QVector<Net> nets;

    // do we need these?
    QVector<net_t> cout;
    QVector<net_t> lout;
    QVector<net_t> lcout;
    QVector<net_t> ioin;
};

Q_DECLARE_METATYPE(ChipDB)

#endif // CHIPDB_H
