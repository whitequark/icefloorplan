#include <QtDebug>
#include <QRegularExpression>
#include "ascparser.h"
#include "chipdb.h"

ChipDB::ChipDB()
    : width(0), height(0)
{}

ChipDB::Tile &ChipDB::tile(coord_t x, coord_t y)
{
    return tiles[qMakePair(x, y)];
}

QMap<QString, net_t> ChipDB::tileNets(coord_t x, coord_t y)
{
    return tilesNets[qMakePair(x, y)];
}

net_t ChipDB::tileNet(coord_t x, coord_t y, const QString &name)
{
    return tilesNets[qMakePair(x, y)][name];
}

static nbit_t parseBitdef(QString bitdef, nbit_t columns)
{
    static const QRegularExpression re("^B([0-9]+)\\[([0-9]+)\\]$");
    QRegularExpressionMatch match = re.match(bitdef);
    if(!match.hasMatch()) {
        qCritical() << "malformed bitdef" << bitdef;
        return (nbit_t)-1;
    }

    nbit_t row = match.capturedRef(1).toInt();
    nbit_t col = match.capturedRef(2).toInt();
    return row * columns + col;
}

bool ChipDB::parse(QIODevice *in, std::function<void(int, int)> progress)
{
    AscParser parser(in);
    while(parser.isOk() && !parser.atEnd()) {
        progress(in->pos(), in->size());

        QString command = parser.parseCommand();
        if(command == "device") {
            QString name = parser.parseName();
            width  = parser.parseDecimal();
            height = parser.parseDecimal();
            size_t num_nets = parser.parseDecimal();
            parser.parseEol();

            Q_ASSERT(this->name == "" || this->name == name);
            this->name = name;

            nets .fill(Net { -1, {} }, num_nets);
            cout .fill(-1, 8 * width * height);
            lout .fill(-1, 7 * width * height);
            lcout.fill(-1, 8 * width * height);
            ioin .fill(-1, 4 * width * height);
        } else if(command == "pins") {
            Package package;
            package.name = parser.parseName();

            while(parser.isOk() && !parser.atCommand()) {
                Pin pin;
                pin.name   = parser.parseName();
                pin.tileX = parser.parseDecimal();
                pin.tileY = parser.parseDecimal();
                pin.net    = parser.parseDecimal();
                parser.parseEol();

                package.pins[pin.name] = pin;
            }

            packages[package.name] = package;
        } else if(command == "gbufin") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "gbufpin") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "iolatch") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "ieren") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "colbuf") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "io_tile"   || command == "logic_tile" ||
                  command == "ramb_tile" || command == "ramt_tile"  ||
                  command == "dsp0_tile" || command == "dsp1_tile"  ||
                  command == "dsp2_tile" || command == "dsp3_tile"  ||
                  command == "ipcon_tile") {
            Tile tile;
            tile.type = command.left(command.indexOf("_"));
            tile.x = parser.parseDecimal();
            tile.y = parser.parseDecimal();
            parser.parseEol();

            this->tile(tile.x, tile.y) = tile;
        } else if(command == "io_tile_bits"   || command == "logic_tile_bits" ||
                  command == "ramb_tile_bits" || command == "ramt_tile_bits"  ||
                  command == "dsp0_tile_bits" || command == "dsp1_tile_bits"  ||
                  command == "dsp2_tile_bits" || command == "dsp3_tile_bits"  ||
                  command == "ipcon_tile_bits") {
            TileBits tile_bits;
            tile_bits.type = command.left(command.indexOf("_"));
            tile_bits.columns = parser.parseDecimal();
            tile_bits.rows    = parser.parseDecimal();
            parser.parseEol();

            while(parser.isOk() && !parser.atCommand()) {
                QString func = parser.parseName();
                QVector<nbit_t> bits;
                while(parser.isOk() && !parser.atEol()) {
                    nbit_t bit = parseBitdef(parser.parseName(), tile_bits.columns);
                    if(bit == (nbit_t)-1) return false;
                    bits.append(bit);
                }

                std::reverse(bits.begin(), bits.end());
                tile_bits.functions[func] = bits;
            }

            tilesBits[tile_bits.type] = tile_bits;
        } else if(command == "extra_cell") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "extra_bits") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "net") {
            Net net;
            net.num = parser.parseDecimal();
            parser.parseEol();

            while(parser.isOk() && !parser.atCommand()) {
                TileNet entry;
                entry.tileX = parser.parseDecimal();
                entry.tileY = parser.parseDecimal();
                entry.name   = parser.parseName();
                parser.parseEol();

                net.tileNets.append(entry);
            }

            nets[net.num] = net;
        } else if(command == "buffer" || command == "routing") {
            Connection conn;
            coord_t tile_x = parser.parseDecimal();
            coord_t tile_y = parser.parseDecimal();
            nbit_t columns = tilesBits[tile(tile_x, tile_y).type].columns;
            conn.dstNet = parser.parseDecimal();
            while(parser.isOk() && !parser.atEol()) {
                nbit_t bit = parseBitdef(parser.parseName(), columns);
                if(bit == (nbit_t)-1) return false;
                conn.bits.append(bit);
            }
            conn.srcNets.fill(-1, 1 << conn.bits.length());
            parser.parseEol();

            while(parser.isOk() && !parser.atCommand()) {
                int config = parser.parseBinary();
                conn.srcNets[config] = parser.parseDecimal();
                parser.parseEol();
            }

            if(command == "buffer") {
                tile(tile_x, tile_y).buffers.append(conn);
            } else if(command == "routing") {
                tile(tile_x, tile_y).routing.append(conn);
            }
        } else {
            qCritical() << "unexpected command" << "." + command;
            return false;
        }
    }

    for(Net &net : nets) {
        for(TileNet &tileNet : net.tileNets) {
            auto coord = qMakePair(tileNet.tileX, tileNet.tileY);
            tilesNets[coord][tileNet.name] = net.num;
        }
    }

    return parser.isOk();
}
