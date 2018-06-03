#ifndef CHIPDB_H
#define CHIPDB_H

#include <QVector>
#include <QMap>
#include <QIODevice>

class ChipDB
{
    struct Pin {
        QString name;
        int tile_x;
        int tile_y;
        int net_num;
    };

    struct Package {
        QString name;
        QMap<QString, Pin> pins;
    };

    struct Connection {
        QVector<int> bits;
        int dst_net_num;
        QVector<int> src_net_nums;
    };

    struct TileBits {
        QString type;
        int rows;
        int columns;
        QMap<QString, QVector<int>> functions;
    };

    struct Tile {
        QString type;
        int x;
        int y;
        QVector<Connection> buffers;
        QVector<Connection> routing;
    };

    struct NetEntry {
        int tile_x;
        int tile_y;
        QString name;
    };

    struct Net {
        int num;
        QString kind;
        QVector<NetEntry> entries;
    };

public:
    ChipDB();
    bool parse(QIODevice *in, std::function<void(int,int)> progress);

    Tile &tile(int x, int y);

    QString name;
    int width;
    int height;
    int num_nets;
    QVector<int> cout;
    QVector<int> lout;
    QVector<int> lcout;
    QVector<int> ioin;
    QMap<QString, Package> packages;
    QMap<QString, TileBits> tiles_bits;
    QMap<QPair<int, int>, Tile> tiles;
    QVector<Net> nets;
};

Q_DECLARE_METATYPE(ChipDB)

#endif // CHIPDB_H
