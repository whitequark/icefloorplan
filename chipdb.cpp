#include <QtDebug>
#include <QRegularExpression>
#include "ascparser.h"
#include "chipdb.h"

ChipDB::ChipDB()
    : width(0), height(0), num_nets(0)
{}

ChipDB::Tile &ChipDB::tile(int x, int y)
{
    return tiles[qMakePair(x, y)];
}

static int parseBitdef(QString bitdef, int columns)
{
    static const QRegularExpression re("^B([0-9]+)\\[([0-9]+)\\]$");
    QRegularExpressionMatch match = re.match(bitdef);
    if(!match.hasMatch()) {
        qCritical() << "malformed bitdef" << bitdef;
        return -1;
    }

    int row = match.capturedRef(1).toInt();
    int col = match.capturedRef(2).toInt();
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
            width    = parser.parseDecimal();
            height   = parser.parseDecimal();
            num_nets = parser.parseDecimal();
            parser.parseEol();

            Q_ASSERT(this->name == "" || this->name == name);
            this->name = name;

            nets .resize(num_nets);
            cout .fill(-1, 8 * width * height);
            lout .fill(-1, 7 * width * height);
            lcout.fill(-1, 8 * width * height);
            ioin .fill(-1, 4 * width * height);
        } else if(command == "pins") {
            Package package;
            package.name = parser.parseName();

            while(parser.isOk() && !parser.atCommand()) {
                Pin pin;
                pin.name    = parser.parseName();
                pin.tile_x  = parser.parseDecimal();
                pin.tile_y  = parser.parseDecimal();
                pin.net_num = parser.parseDecimal();
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
                QVector<int> bits;
                while(parser.isOk() && !parser.atEol()) {
                    int bit = parseBitdef(parser.parseName(), tile_bits.columns);
                    if(bit == -1) return false;
                    bits.append(bit);
                }

                tile_bits.functions[func] = bits;
            }

            tiles_bits[tile_bits.type] = tile_bits;
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
                NetEntry entry;
                entry.tile_x = parser.parseDecimal();
                entry.tile_y = parser.parseDecimal();
                entry.name   = parser.parseName();
                parser.parseEol();

                const QString &name = entry.name;
                if(name.startsWith("sp12_h")) {
                    net.kind = "sp12h";
                } else if(name.startsWith("sp12_v")) {
                    net.kind = "sp12v";
                } else if(name.startsWith("sp4_h")) {
                    net.kind = "sp4h";
                } else if(name.startsWith("sp4_v")) {
                    net.kind = "sp4v";
                } else if(name.startsWith("fabout")) {
                    net.kind = "fb";
                } else if(name.startsWith("glb_netwk")) {
                    net.kind = "glb";
                } else if(name.startsWith("io_") && name.mid(4, 6) == "/D_IN_") {
                    net.kind = "ioin";
                    // Save for later the net number for IO IN pin.
                    int pad = name.mid(3, 1).toInt();
                    int pin = name.mid(10, 1).toInt();
                    int idx = pin + 2 * pad + 4 * (entry.tile_x + width * entry.tile_y);
                    ioin[idx] = net.num;
                } else if(name.startsWith("io_0/D_OUT_0")) {
                    net.kind = "ioou";
                } else if(name.startsWith("io_0/D_OUT_1")) {
                    net.kind = "ioou";
                } else if(name.startsWith("io_1/D_OUT_0")) {
                    net.kind = "ioou";
                } else if(name.startsWith("io_1/D_OUT_1")) {
                    net.kind = "ioou";
                } else if(name.startsWith("io_0/OUT_ENB")) {
                    net.kind = "ioou";
                } else if(name.startsWith("io_1/OUT_ENB")) {
                    net.kind = "ioou";
                } else if(name.startsWith("io_global/cen")) {
                    net.kind = "iocen";
                } else if(name.startsWith("io_global/inclk")) {
                    net.kind = "ioclki";
                } else if(name.startsWith("io_global/outclk")) {
                    net.kind = "ioclko";
                } else if(name.startsWith("ram/RDATA_")) {
                    net.kind = "rdat";
                } else if(name.startsWith("ram/WDATA_")) {
                    net.kind = "wdat";
                } else if(name.startsWith("ram/RADDR_")) {
                    net.kind = "radr";
                } else if(name.startsWith("ram/WADDR_")) {
                    net.kind = "wadr";
                } else if(name.startsWith("ram/MASK_")) {
                    net.kind = "mask";
                } else if(entry.name == "ram/RCLK") {
                    net.kind = "rclk";
                } else if(entry.name == "ram/RCLKE") {
                    net.kind = "rclke";
                } else if(entry.name == "ram/RE") {
                    net.kind = "we";
                } else if(entry.name == "ram/WCLK") {
                    net.kind = "wclk";
                } else if(entry.name == "ram/WCLKE") {
                    net.kind = "wclke";
                } else if(entry.name == "ram/WE") {
                    net.kind = "re";
                } else if(name.startsWith("local_g")) {
                    net.kind = "loc";
                } else if(name.startsWith("glb2local")) {
                    net.kind = "g2l";
                } else if(name.startsWith("lutff_") && name.mid(7, 4) == "/in_") {
                    net.kind = "lcin";   // Logic cell input
                } else if(name.startsWith("lutff_") && name.endsWith("/out")) {
                    net.kind = "lcout";   // Logic cell output
                    // Save for later the net number for DFF out.
                    int lut = name.mid(6, 1).toInt();
                    int idx = lut + 8 * (entry.tile_x + width * entry.tile_y);
                    lcout[idx] = net.num;
                } else if(name.startsWith("lutff_") && name.endsWith("/lout")) {
                    net.kind = "lout";   // Logic cell output pre-flipflop
                    // Save for later the net number for LUT-out.
                    int lut = name.mid(6, 1).toInt();
                    if(lut >= 7) {
                        qCritical() << "unexpected LUT 7 lout" << entry.name;
                        return false;
                    }
                    int idx = lut + 7 * (entry.tile_x + width * entry.tile_y);
                    lout[idx] = net.num;
                } else if(name.startsWith("lutff_") && name.endsWith("/cout")) {
                    net.kind = "cout";    // Carry output
                    // Save for later the net number for carry-out.
                    int lut = name.mid(6, 1).toInt();
                    int idx = lut + 8 * (entry.tile_x + width * entry.tile_y);
                    cout[idx] = net.num;
                } else if(entry.name == "carry_in") {
                    net.kind = "cin";
                } else if(entry.name == "carry_in_mux") {
                    net.kind = "cmuxin";
                } else if(entry.name == "lutff_global/cen") {
                    net.kind = "lcen";
                } else if(entry.name == "lutff_global/clk") {
                    net.kind = "lclk";
                } else if(entry.name == "lutff_global/s_r") {
                    net.kind = "lsr";
                } else if(name.startsWith("span4_vert_b_") ||
                           name.startsWith("span4_vert_t_") ||
                           name.startsWith("span4_horz_l_") ||
                           name.startsWith("span4_horz_r_")) {
                    net.kind = "iosp4";
                }

                net.entries.append(entry);
            }

            nets[net.num] = net;
        } else if(command == "buffer" || command == "routing") {
            Connection conn;
            int tile_x = parser.parseDecimal();
            int tile_y = parser.parseDecimal();
            int columns = tiles_bits[tile(tile_x, tile_y).type].columns;
            conn.dst_net_num = parser.parseDecimal();
            while(parser.isOk() && !parser.atEol()) {
                int bit = parseBitdef(parser.parseName(), columns);
                if(bit == -1) return false;
                conn.bits.append(bit);
            }
            conn.src_net_nums.fill(-1, 1 << conn.bits.length());
            parser.parseEol();

            while(parser.isOk() && !parser.atCommand()) {
                int config = parser.parseBinary();
                conn.src_net_nums[config] = parser.parseDecimal();
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

    return parser.isOk();
}
