#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include "floorplanbuilder.h"
#include "circuitbuilder.h"

static const qreal GRID = 20;

static const QColor BLOCK_COLOR = Qt::darkRed;
static const QColor NET_COLOR   = Qt::darkGreen;

static const QColor TILE_INACTIVE_COLOR = QColor::fromRgb(0xC0C0C0);
static const QColor TILE_IO_COLOR       = QColor::fromRgb(0xEAFBFB);
static const QColor TILE_LOGIC_COLOR    = QColor::fromRgb(0xFBEAFB);
static const QColor TILE_RAM_COLOR      = QColor::fromRgb(0xFBFBEA);

FloorplanBuilder::FloorplanBuilder(ChipDB *chipDB, Bitstream *bitstream, QGraphicsScene *scene,
                                   LUTNotation lutNotation, bool showUnusedLogic)
    : _lutNotation(lutNotation), _showUnusedLogic(showUnusedLogic), _chip(chipDB),
      _bitstream(bitstream), _scene(scene)
{}

void FloorplanBuilder::buildTiles()
{
    if(!_bitstream) return;

    for(auto &tile : _bitstream->tiles) {
        buildTile(tile);
    }
}

void FloorplanBuilder::buildTile(const Bitstream::Tile &tile)
{
    const qreal WIDTH  = 75;
    const qreal HEIGHT = 75;

    QGraphicsRectItem *tileItem =
        _scene->addRect(QRectF(QPointF(-8, -8) * GRID, QSizeF(WIDTH - 16, HEIGHT - 16) * GRID),
                        Qt::NoPen, Qt::NoBrush);
    tileItem->setPos(QPointF(tile.x * WIDTH, (_chip->height - tile.y) * HEIGHT) * GRID);

    QGraphicsSimpleTextItem *coordsItem = _scene->addSimpleText(
        QString("%3 (%1 %2)").arg(tile.x).arg(tile.y).arg(tile.type), QFont("sans", 18));
    coordsItem->setParentItem(tileItem);
    coordsItem->setPos(QPointF(-8, -10) * GRID);

    if(tile.type == "logic") {
        buildLogicTile(tile, tileItem);
    } else if(tile.type == "io") {
        buildIOTile(tile, tileItem);
    } else if(tile.type == "ramb" || tile.type == "ramt") {
        buildRAMTile(tile, tileItem);
    }
}

QString FloorplanBuilder::recognizeFunction(uint fullLutData, bool hasA, bool hasB, bool hasC,
                                            bool hasD, bool describeInputs) const
{
    uint lutData = 0;
    for(uint i = 0; i < 16; i++) {
        uint si = 0;
        si |= (i & (hasA << 0));
        si |= (i & (hasB << 1));
        si |= (i & (hasC << 2));
        si |= (i & (hasD << 3));

        uint di = 0;
        di |= (i & (hasA << 0)) >> 0;
        di |= (i & (hasB << 1)) >> (!hasA);
        di |= (i & (hasC << 2)) >> (!hasA + !hasB);
        di |= (i & (hasD << 3)) >> (!hasA + !hasB + !hasC);

        if(fullLutData & (1 << si)) lutData |= (1 << di);
    }

    uint inputs    = hasA + hasB + hasC + hasD;
    uint inputBits = 1 << inputs;
    uint inputMask = (1 << inputBits) - 1;

    auto describeRaw = [=] {
        QString lutDataAsc = QString::number(lutData, 2).rightJustified(inputBits, '0');
        std::reverse(lutDataAsc.begin(), lutDataAsc.end());
        for(int i = lutDataAsc.length() - 4; i >= 4; i -= 4)
            lutDataAsc.insert(i, '\n');
        return lutDataAsc;
    };

    auto describe = [=](const QString &prefix, const QString &oper, const QString &compact) {
        QString desc;
        if(_lutNotation == RawLUTs) {
            return describeRaw();
        } else if(_lutNotation == CompactLUTs) {
            return prefix + compact;
        } else if(describeInputs) {
            if(hasA) {
                desc += "A";
            }
            if(hasB) {
                if(!desc.isEmpty()) desc += oper;
                desc += "B";
            }
            if(hasC) {
                if(!desc.isEmpty()) desc += oper;
                desc += "C";
            }
            if(hasD) {
                if(!desc.isEmpty()) desc += oper;
                desc += "D";
            }
            return prefix + desc;
        } else {
            return prefix + oper;
        }
    };

    if(lutData == 0) {
        return "0";
    } else if(lutData == inputMask) {
        return "1";
    } else if(inputs == 1 && lutData == 0b10) { // BUF
        return describe("", "", "1");
    } else if(inputs == 1 && lutData == 0b01) { // NOT
        return describe("~", "", "1");
    } else if(inputs == 2 && lutData == 0b0110) { // XOR
        return describe("", "⊕", "=1");
    } else if(inputs == 2 && lutData == 0b1001) { // XNOR
        return describe("~", "⊕", "=1");
    } else if(inputs == 3 && lutData == 0b10010110) { // full adder
        return describe("∑", "", "");
    } else if(lutData == (1 << (inputBits - 1))) { // AND
        return describe("", "·", "&");
    } else if(lutData == ((1 << (inputBits - 1)) ^ inputMask)) { // NAND
        return describe("~", "·", "&");
    } else if(lutData == ((1 << inputBits) - 2)) { // OR
        return describe("", "+", "≥1");
    } else if(lutData == (((1 << inputBits) - 2) ^ inputMask)) { // NOR
        return describe("~", "+", "≥1");
    } else {
        return describeRaw();
        // return "𝑓";
    }
}

void FloorplanBuilder::buildLogicTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem)
{
    CircuitBuilder builder(tileItem);
    builder.setGrid(GRID);

    const auto &tileNets   = _chip->tileNets(tile.x, tile.y);
    const auto &tileBits   = _chip->tilesBits["logic"];
    const auto &netDrivers = _bitstream->netDrivers;
    const auto &netLoaded  = _bitstream->netLoaded;

    // See topology and bitstream documentation at:
    //  * http://www.clifford.at/icestorm/logic_tile.html
    //  * http://www.clifford.at/icestorm/bitdocs-1k/tile_6_9.html

    // Variables used in this function:
    //  * net name as a string, used to fetch net properties
    //    QString <net_name> = "<net_name>";
    //  * chipdb net number corresponding to <net_name> (n_ stands for net)
    //    net_t n_<net_name> = tileNets[<net_name>];
    //  * net that is driving <net_name> or -1 if it's not driven (d_ stands for driver)
    //    net_t d_<net_name> = netDrivers[n_<net_name>];
    //  * whether <net_name> is driving any other net (l_ stands for loaded)
    //    bool  l_<net_name> = netLoaded[n_<net_name>];
    // NB: each tile has same net names but almost always different global nets
    // connected to them.

    // All FFs in the tile have the same clock polarity.
    bool negClk = tile.extract(tileBits.functions["NegClk"]);

    // All FFs in the tile share the clock (clk), enable (cen) and set/reset (s_r)
    // nets. Whether the FF is enabled, whether the set/reset line sets or resets
    // the FF, and whether the set/reset is synchronous or asynchronous is determined
    // per individual FF.
    QString lutff_global_clk = "lutff_global/clk";
    net_t n_lutff_global_clk = tileNets[lutff_global_clk];
    net_t d_lutff_global_clk = netDrivers[n_lutff_global_clk];
    QString lutff_global_cen = "lutff_global/cen";
    net_t n_lutff_global_cen = tileNets[lutff_global_cen];
    net_t d_lutff_global_cen = netDrivers[n_lutff_global_cen];
    QString lutff_global_s_r = "lutff_global/s_r";
    net_t n_lutff_global_s_r = tileNets[lutff_global_s_r];
    net_t d_lutff_global_s_r = netDrivers[n_lutff_global_s_r];

    // The net carry_in in tile (x, y) is connected to lutff_7/cout in tile (x, y-1).
    // The net carry_in_mux may be driven by carry_in or by a constant determined
    // by the bitstream bit CarryInSet.
    QString carry_in     = "carry_in";
    net_t n_carry_in     = tileNets[carry_in];
    net_t d_carry_in     = netDrivers[n_carry_in];
    QString carry_in_mux = "carry_in_mux";
    net_t d_carry_in_mux = netDrivers[tileNets[carry_in_mux]];
    bool carryInSet      = tile.extract(tileBits.functions["CarryInSet"]);

    // hasCarryIn determines whether we have carry in from either the previous logic cell,
    // the tile to the bottom, or a constant driver.
    // If it's set, carryIn has the coordinates of the output pin of the driver.
    bool hasCarryIn = false;
    QPointF carryIn;

    // Draw the carry in driver (if any).
    builder.setColor(BLOCK_COLOR);
    builder.setOrigin(1, 8 * 6 - 1.5);
    if(d_carry_in_mux != -1) {
        // Draw a buffer from carry out of tile (x, y-1) to this tile's carry in.
        hasCarryIn = true;
        builder.addBuffer(CircuitBuilder::Up, 1, 1);
        carryIn            = builder.addPin(CircuitBuilder::Up, 1, 0);
        QPointF fabCarryIn = builder.addPin(CircuitBuilder::Down, 1, 0);
        builder.build("carry_in");

        // TODO: replace this stub carry_in net with a real one.
        if(d_carry_in != -1) {
            builder.setColor(NET_COLOR);
            builder.moveTo(fabCarryIn);
            builder.wireTo(fabCarryIn + QPointF(0, 1));
            builder.build("carry_in", n_carry_in);
        }
    } else if(carryInSet) {
        // Draw a constant driver.
        hasCarryIn = true;
        builder.addBuffer(CircuitBuilder::Up, 1, 1);
        builder.addLabel(CircuitBuilder::Up, 1, 0.1, "1");
        carryIn = builder.addPin(CircuitBuilder::Up, 1, 0);
        builder.build("carry_in");
    }

    // Is there any LUT whose output is loaded?
    bool isActive = false;

    // Draw all logic cells and collect coordinates of their inputs and outputs
    // so that tile-wide connections can be drawn.
    QVector<QPointF> ffENs, ffCLKs, ffSRs;
    for(int lc = 0; lc < 8; lc++) {
        // Y offset of the logic cell
        qreal lcOff = (7 - lc) * 6;

        // Each logic cell has a set of tile bits configuring it. These bits
        // include the LUT truth table, the FF and carry configuration in what
        // appears to be a modified Hilbert curve, so there's no straightforward
        // mapping to anything useful; it's also not documented anywhere.
        uint lutffConfig = tile.extract(tileBits.functions[QString("LC_%1").arg(lc)]);

        // Extract LUT truth table.
        // For any binary digits ABCD, LUT[ABCD]=(lutData>>0bABCD)&1.
        uint lutData = 0;
        for(nbit_t nbit : {4, 14, 15, 5, 6, 16, 17, 7, 3, 13, 12, 2, 1, 11, 10, 0}) {
            lutData >>= 1;
            if(lutffConfig & (1 << nbit)) lutData |= 1 << 15;
        }

        // Whether this logic cell's carry unit is enabled. If disabled, the carry
        // unit always outputs 0.
        bool hasCarryOut = lutffConfig & (1 << 8);
        // Whether this logic cell's DFF is enabled. If disabled, the logic cell
        // output is connected to LUT output.
        bool hasDFF = lutffConfig & (1 << 9);

        // If true, this logic cell's DFF has a set input. If false, reset input.
        bool srSet = lutffConfig & (1 << 18);
        // If true, this logic cell's DFF uses an asynchronous set/reset input. If false,
        // synchronous.
        bool asyncSR = lutffConfig & (1 << 19);

        // Nets internal to this logic cell.
        QString lutff      = QString("lutff_%1").arg(lc);
        QString lutff_in0  = lutff + "/in_0";
        net_t n_lutff_in0  = tileNets[lutff_in0];
        net_t d_lutff_in0  = netDrivers[n_lutff_in0];
        QString lutff_in1  = lutff + "/in_1";
        net_t n_lutff_in1  = tileNets[lutff_in1];
        net_t d_lutff_in1  = netDrivers[n_lutff_in1];
        QString lutff_in2  = lutff + "/in_2";
        net_t n_lutff_in2  = tileNets[lutff_in2];
        net_t d_lutff_in2  = netDrivers[n_lutff_in2];
        QString lutff_in3  = lutff + "/in_3";
        net_t n_lutff_in3  = tileNets[lutff_in3];
        net_t d_lutff_in3  = netDrivers[n_lutff_in3];
        QString lutff_cin  = lc > 0 ? QString("lutff_%1/cout").arg(lc - 1) : "carry_in_mux";
        net_t n_lutff_cin  = tileNets[lutff_cin];
        QString lutff_cout = lutff + "/cout";
        QString lutff_lin  = lc == 7 ? "" : QString("lutff_%1/lout").arg(lc - 1);
        net_t n_lutff_lin  = lc == 7 ? -1 : tileNets[lutff_lin];
        QString lutff_lout = lutff + "/lout";
        net_t n_lutff_lout = tileNets[lutff_lout];
        bool l_lutff_lout  = netLoaded[n_lutff_lout];
        QString lutff_out  = lutff + "/out";
        net_t n_lutff_out  = tileNets[lutff_out];
        bool l_lutff_out   = netLoaded[n_lutff_out];

        isActive |= hasDFF || l_lutff_lout || l_lutff_out;

        // Whether we should draw the LUT.
        bool drawLUT = _showUnusedLogic || hasDFF || l_lutff_lout || l_lutff_out;
        // Whether we should draw the carry unit.
        bool drawCarry = _showUnusedLogic || hasCarryOut;
        // Whether we should draw logic cell output.
        bool hasOut = _showUnusedLogic || l_lutff_out;

        // Whether anything is connected to the LUT inputs, and carry unit inputs
        // that are connected to the LUT inputs.
        bool hasA   = d_lutff_in0 != -1;
        bool hasB   = d_lutff_in1 != -1;
        bool hasC   = d_lutff_in2 != -1;
        bool hasD   = d_lutff_in3 != -1;
        uint inputs = hasA + hasB + hasC + hasD;

        // Draw this logic cell's carry unit.
        QPointF carryI0, carryCI, carryI1, carryO;
        builder.setColor(BLOCK_COLOR);
        builder.setOrigin(1, lcOff - 1.5);
        if(drawCarry) {
            builder.addMux(CircuitBuilder::Up, 1.5, 0, 3);
            builder.addLabel(CircuitBuilder::Up, 1, 0, "∑₁");
            if(d_lutff_in2 != -1) carryI0 = builder.addPin(CircuitBuilder::Down, 0, 0);
            if(hasCarryIn) carryCI = builder.addPin(CircuitBuilder::Down, 1, 0);
            if(d_lutff_in1 != -1) carryI1 = builder.addPin(CircuitBuilder::Down, 2, 0);
            if(hasCarryOut) carryO = builder.addPin(CircuitBuilder::Up, 1, 0);
        }
        builder.build(lutff + "/carry");

        // Draw this logic cell's LUT.
        QPointF lutI0, lutI1, lutI2, lutI3, lutO;
        builder.setColor(BLOCK_COLOR);
        builder.setOrigin(5, lcOff);
        if(drawLUT && inputs == 0 && !_showUnusedLogic && _lutNotation != RawLUTs) {
            builder.addBuffer(CircuitBuilder::Right, 3, 0);
            builder.addLabel(CircuitBuilder::Left, 3, 0, (lutData & 1) ? "1" : "0");
            lutO = builder.addPin(CircuitBuilder::Right, 3, 0);
        } else if(drawLUT) {
            builder.addBlock(0, 0, 8, 4);
            QString functionDescr = recognizeFunction(lutData, hasA, hasB, hasC, hasD);
            builder.addText(4, 2, functionDescr, functionDescr.contains('\n') ? 1.2 : 1.5);
            if(hasA) lutI0 = builder.addPin(CircuitBuilder::Left, 0, 0, "A");
            if(hasB) lutI1 = builder.addPin(CircuitBuilder::Left, 0, 1, "B");
            if(hasC) lutI2 = builder.addPin(CircuitBuilder::Left, 0, 2, "C");
            if(hasD) lutI3 = builder.addPin(CircuitBuilder::Left, 0, 3, "D");
            lutO = builder.addPin(CircuitBuilder::Right, 7, 0, "O");
        }
        builder.build(lutff + "/lut");

        // Draw this logic cell's FF, or buffer if FF is bypassed.
        QPointF ffD, ffEN, ffCLK, ffQ, ffSR;
        builder.setColor(BLOCK_COLOR);
        builder.setOrigin(18, lcOff);
        if(hasDFF) {
            // Draw an FF.
            if(!asyncSR && (d_lutff_global_s_r != -1 || _showUnusedLogic)) {
                // FF with synchronous set/reset, 4 left-side inputs.
                builder.addBlock(0, 0, 3, 4);
            } else {
                // FF with asynchronous set/reset, 3 left-side inputs and 1 bottom input.
                builder.addBlock(0, 0, 3, 3);
            }
            ffD = builder.addPin(CircuitBuilder::Left, 0, 0, "D");
            ffQ = builder.addPin(CircuitBuilder::Right, 2, 0, "Q");
            if(d_lutff_global_clk != -1 || _showUnusedLogic) {
                // Although degenerate, an FF without a clock could do
                // something useful if it has an asynchronous set input.
                ffCLK = builder.addPin(CircuitBuilder::Left, 0, 1, ">", negClk);
                ffCLKs.append(ffCLK);
            }
            if(d_lutff_global_cen != -1 || _showUnusedLogic) {
                ffEN = builder.addPin(CircuitBuilder::Left, 0, 2, "E");
                ffENs.append(ffEN);
            }
            if(d_lutff_global_s_r != -1 || _showUnusedLogic) {
                const char *label = srSet ? "S" : "R";
                if(asyncSR) {
                    ffSR = builder.addPin(CircuitBuilder::Down, 1, 2, label);
                } else {
                    ffSR = builder.addPin(CircuitBuilder::Left, 0, 3, label);
                }
                ffSRs.append(ffSR);
            }
        } else if(hasOut) {
            // Draw a buffer.
            builder.addBuffer(CircuitBuilder::Right, 0, 0);
            ffD = builder.addPin(CircuitBuilder::Left, 0, 0);
            ffQ = builder.addPin(CircuitBuilder::Right, 0, 0);
        }
        builder.build(lutff + "/ff");

        // Draw nets connecting the LUT inputs and carry adder inputs.
        builder.setColor(NET_COLOR);
        builder.setOrigin(0, lcOff);

        // Draw input A net. This is connected only to LUT I0, and is driven by
        // a local track.
        if(hasA) {
            QPointF lutffI0 = builder.moveTo(0, 0);
            if(drawLUT) {
                builder.wireTo(lutI0);
            }
            builder.build(lutff_in0, n_lutff_in0);
        }

        // Draw input B net. This is connected to LUT I1 and carry unit I1,
        // and is driven by a local track.
        if(hasB) {
            QPointF lutffI1 = builder.moveTo(0, 1);
            if(drawLUT) {
                builder.wireTo(lutI1);
            }
            if(drawCarry) {
                builder.joinTo(QPointF(carryI1.x(), lutffI1.y()), drawLUT);
                builder.wireTo(carryI1);
            }
            builder.build(lutff_in1, n_lutff_in1);
        }

        // Draw input C net. This is connected to LUT I2, carry unit I0,
        // and is driven by either LUT output of the previous logic cell
        // (if this isn't the first cell in the tile), or a local track.
        if(hasC) {
            builder.moveTo(lutI2);
            if(n_lutff_in2 == n_lutff_lin) {
                // lutff_(N-1)/lout case
                builder.wireTo(3, 2);
            } else if(drawLUT) {
                QPointF lutffI2 = builder.wireTo(0, 2);
            }
            if(drawCarry) {
                builder.moveTo(carryI0);
                builder.wireTo(1, 2);
                if(n_lutff_in2 == n_lutff_lin) {
                    // lutff_(N-1)/lout case
                    builder.wireTo(3, 2);
                    builder.junction(drawLUT);
                } else if(drawLUT) {
                    builder.junction();
                } else {
                    QPointF lutffI2 = builder.wireTo(0, 2);
                }
            }
            builder.build(lutff_in2, n_lutff_in2);
        }

        // Draw input D net. This is connected to LUT I3 and may be driven by
        // the same net that drives carry unit CI, or a local track.
        if(hasD) {
            if(d_lutff_in3 == n_lutff_cin) {
                if(drawLUT && drawCarry) {
                    builder.junctionTo(QPointF(carryIn.x(), lutI3.y()));
                } else {
                    builder.moveTo(QPointF(carryIn.x(), lutI3.y()));
                }
            } else {
                QPointF lutffI3 = builder.moveTo(0, 3);
            }
            if(drawLUT) {
                builder.wireTo(lutI3);
            }
            builder.build(lutff_in3, n_lutff_in3);
        }

        // Draw carry unit input net.
        if(hasCarryIn) {
            builder.moveTo(carryIn);
            if(drawCarry) {
                builder.wireTo(carryCI);
            } else if(drawLUT) {
                builder.wireTo(QPointF(carryIn.x(), lutI3.y()));
            }
            builder.build(lutff_cin, n_lutff_cin);
        }

        // Draw LUT output net. This is connected to DFF or output buffer,
        // and may drive LUT I2 input of the next logic cell.
        if(drawLUT) {
            builder.moveTo(lutO);
            builder.wireTo(ffD);
            if(l_lutff_lout) {
                builder.junctionTo(13.5, 0);
                builder.wireTo(13.5, -2);
                builder.wireTo(3, -2);
                builder.wireTo(3, -4);
            }
            builder.build(lutff_lout, n_lutff_lout);
        }

        // Draw logic cell output net. This is driven by DFF or output buffer.
        if(l_lutff_out) {
            builder.moveTo(ffQ);
            builder.wireTo(22, 0);
            builder.build(lutff_out, n_lutff_out);
        }

        // Remember the carry unit configuration to draw the next logic cell.
        hasCarryIn = hasCarryOut;
        carryIn    = carryO;
    }

    // If the per-tile FF input nets have a non-constant driver and
    // any FFs are enabled, or unused logic is shown, draw the per-tile nets.
    auto drawTileFFNet = [&](qreal originX, qreal originY, const QVector<QPointF> &ffNets,
                             const QString &netName, net_t net) {
        if(ffNets.isEmpty()) return QPointF();

        builder.setOrigin(originX, originY);

        if(d_lutff_global_clk == -1) {
            // Draw a constant driver.
            builder.setColor(BLOCK_COLOR);
            builder.addBuffer(CircuitBuilder::Right, -1, 0);
            builder.addLabel(CircuitBuilder::Left, -1, 0, "0");
            builder.addPin(CircuitBuilder::Right, -1, 0);
            builder.build();
        }

        // Start drawing vertical segment.
        builder.setColor(NET_COLOR);
        QPointF tileNet = builder.moveTo(0, 0);

        // Draw horizontal segments to FFs.
        qreal maxY = 0;
        for(QPointF ffNet : ffNets) {
            builder.junctionTo(QPointF(tileNet.x(), ffNet.y()));
            builder.wireTo(ffNet);
            maxY = qMax(maxY, ffNet.y());
        }

        // Finish drawing vertical segment.
        builder.moveTo(tileNet);
        builder.wireTo(QPointF(tileNet.x(), maxY));
        builder.build(netName, net);

        return tileNet;
    };

    drawTileFFNet(16, -5, ffCLKs, lutff_global_clk, n_lutff_global_clk);
    drawTileFFNet(15, -4, ffENs, lutff_global_cen, n_lutff_global_cen);
    drawTileFFNet(14, -3, ffSRs, lutff_global_s_r, n_lutff_global_s_r);

    tileItem->setBrush(QBrush(isActive ? TILE_LOGIC_COLOR : TILE_INACTIVE_COLOR));
}

void FloorplanBuilder::buildIOTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem)
{
    bool isActive = true;

    tileItem->setBrush(QBrush(isActive ? TILE_IO_COLOR : TILE_INACTIVE_COLOR));
}

void FloorplanBuilder::buildRAMTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem)
{
    bool isActive = true;

    tileItem->setBrush(QBrush(isActive ? TILE_RAM_COLOR : TILE_INACTIVE_COLOR));
}
