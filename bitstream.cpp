#include <QtDebug>
#include "ascparser.h"
#include "bitstream.h"

Bitstream::Bitstream()
{}

Bitstream::Tile &Bitstream::tile(coord_t x, coord_t y)
{
    return tiles[qMakePair(x, y)];
}

bool Bitstream::parse(QIODevice *in, std::function<void(int,int)> progress)
{
    AscParser parser(in);
    while(parser.isOk() && !parser.atEnd()) {
        progress(in->pos(), in->size());

        QString command = parser.parseCommand();
        if(command == "comment") {
            comment = parser.parseRest();
        } else if(command == "device") {
            device = parser.parseName();
            parser.parseEol();
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

            while(parser.isOk() && !parser.atCommand()) {
                QString bitsAsc = parser.parseRest();
                size_t appendAt = tile.bits.count();
                tile.bits.resize(appendAt + bitsAsc.length());
                for(QChar bitAsc : bitsAsc) {
                    if(bitAsc == '0') {
                        tile.bits.clearBit(appendAt++);
                    } else if(bitAsc == '1') {
                        tile.bits.setBit(appendAt++);
                    } else {
                        qCritical() << "bit" << appendAt << "not 0 or 1:" << bitAsc;
                        return false;
                    }
                }
            }

            tiles[qMakePair(tile.x, tile.y)] = tile;
        } else if(command == "ram_data") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "extra_bit") {
            // not implemented
            parser.skipToCommand();
        } else if(command == "sym") {
            net_t net = parser.parseDecimal();
            QString name = parser.parseName();
            parser.parseEol();

            symbols[net] = name;
        } else {
            qCritical() << "unexpected command" << "." + command;
            return false;
        }
    }

    return parser.isOk();
}

uint Bitstream::Tile::extract(const QVector<nbit_t> &nbits) const
{
    uint result = 0;
    for(nbit_t nbit : nbits) {
        result <<= 1;
        result |= bits[nbit];
    }
    return result;
}

bool Bitstream::process(ChipDB &chip)
{
    netDrivers.fill(-1, chip.nets.length());
    netLoaded.resize(chip.nets.length());

    for(Tile &tile : tiles) {
        if(!chip.tiles.contains(qMakePair(tile.x, tile.y))) {
            qCritical() << "tile at" << tile.x << tile.y << "does not exist";
            return false;
        }

        if(chip.tile(tile.x, tile.y).type != tile.type) {
            qCritical() << "tile at" << tile.x << tile.y << "has wrong type" << tile.type;
            return false;
        }

        ChipDB::TileBits &tileBits = chip.tilesBits[tile.type];
        if(tileBits.rows * tileBits.columns != tile.bits.count()) {
            qCritical() << "tile at" << tile.x << tile.y << "has wrong bit count" << tile.bits.count();
            return false;
        }

        ChipDB::Tile &chipTile = chip.tile(tile.x, tile.y);
        for(ChipDB::Connection &buffer : chipTile.buffers) {
            uint config = tile.extract(buffer.bits);
            net_t srcNet = buffer.srcNets[config];
            if(srcNet != (net_t)-1) {
                if(netDrivers[buffer.dstNet] != (net_t)-1) {
                    qCritical() << "net" << buffer.dstNet << "is driven by net"
                                << netDrivers[buffer.dstNet] << "and"
                                << srcNet;
                    return false;
                }
                netDrivers[buffer.dstNet] = srcNet;
                netLoaded.setBit(srcNet);
            }
        }
    }

    return true;
}

