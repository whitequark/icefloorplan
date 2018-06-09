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

    bool negClk = tile.extract(tileBits.functions["NegClk"]);

    QString lutff_global_cen = "lutff_global/cen";
    net_t n_lutff_global_cen = tileNets[lutff_global_cen];
    net_t d_lutff_global_cen = netDrivers[n_lutff_global_cen];
    QString lutff_global_clk = "lutff_global/clk";
    net_t n_lutff_global_clk = tileNets[lutff_global_clk];
    net_t d_lutff_global_clk = netDrivers[n_lutff_global_clk];
    QString lutff_global_s_r = "lutff_global/s_r";
    net_t n_lutff_global_s_r = tileNets[lutff_global_s_r];
    net_t d_lutff_global_s_r = netDrivers[n_lutff_global_s_r];

    QString carry_in_mux = "carry_in_mux";
    net_t d_carry_in_mux = netDrivers[tileNets[carry_in_mux]];
    QString carry_in     = "carry_in";
    net_t n_carry_in     = tileNets[carry_in];
    net_t d_carry_in     = netDrivers[n_carry_in];

    bool hasCarryIn = false;
    QPointF carryIn;
    builder.setColor(BLOCK_COLOR);
    builder.setOrigin(1, 8 * 6 - 1.5);
    if(tile.extract(tileBits.functions["CarryInSet"])) {
        hasCarryIn = true;
        builder.addBuffer(CircuitBuilder::Up, 1, 1);
        builder.addLabel(CircuitBuilder::Up, 1, 0.1, "1");
        carryIn = builder.addPin(CircuitBuilder::Up, 1, 0);
        builder.build("carry_in");
    } else if(d_carry_in_mux != -1) {
        hasCarryIn = true;
        builder.addBuffer(CircuitBuilder::Up, 1, 1);
        carryIn            = builder.addPin(CircuitBuilder::Up, 1, 0);
        QPointF fabCarryIn = builder.addPin(CircuitBuilder::Down, 1, 0);
        builder.build("carry_in");

        if(d_carry_in != -1) {
            builder.setColor(NET_COLOR);
            builder.moveTo(fabCarryIn);
            builder.wireTo(fabCarryIn + QPointF(0, 1));
            builder.build("carry_in", n_carry_in);
        }
    }

    bool isActive = false;

    QVector<QPointF> ffENs, ffCLKs, ffSRs;
    for(int lc = 0; lc < 8; lc++) {
        qreal lcOff = (7 - lc) * 6;

        uint lutConfig = tile.extract(tileBits.functions[QString("LC_%1").arg(lc)]);

        uint lutData = 0;
        for(nbit_t nbit : {4, 14, 15, 5, 6, 16, 17, 7, 3, 13, 12, 2, 1, 11, 10, 0}) {
            lutData >>= 1;
            if(lutConfig & (1 << nbit)) lutData |= 1 << 15;
        }

        bool cfgCarry = lutConfig & (1 << 8);
        bool hasDFF   = lutConfig & (1 << 9);
        bool srSet    = lutConfig & (1 << 18);
        bool asyncSR  = lutConfig & (1 << 19);

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

        bool hasLUT   = _showUnusedLogic || hasDFF || l_lutff_lout || l_lutff_out;
        bool hasCarry = _showUnusedLogic || cfgCarry;
        bool hasOut   = _showUnusedLogic || l_lutff_out;
        isActive |= hasDFF || l_lutff_lout || l_lutff_out;

        bool hasA   = d_lutff_in0 != -1;
        bool hasB   = d_lutff_in1 != -1;
        bool hasC   = d_lutff_in2 != -1;
        bool hasD   = d_lutff_in3 != -1;
        uint inputs = hasA + hasB + hasC + hasD;

        builder.setColor(BLOCK_COLOR);
        // lutff_N (carry adder)
        QPointF carryI0, carryI1, carryI2, carryO;
        builder.setOrigin(1, lcOff - 1.5);
        if(hasCarry) {
            builder.addMux(CircuitBuilder::Up, 1.5, 0, 3);
            builder.addLabel(CircuitBuilder::Up, 1, 0, "∑₁");
            if(d_lutff_in2 != -1) carryI0 = builder.addPin(CircuitBuilder::Down, 0, 0);
            if(hasCarryIn) carryI1 = builder.addPin(CircuitBuilder::Down, 1, 0);
            if(d_lutff_in1 != -1) carryI2 = builder.addPin(CircuitBuilder::Down, 2, 0);
            if(cfgCarry) carryO = builder.addPin(CircuitBuilder::Up, 1, 0);
        }
        builder.build(lutff + "/carry");
        // lutff_N (look-up table)
        QPointF lutI0, lutI1, lutI2, lutI3, lutO;
        builder.setOrigin(5, lcOff);
        if(hasLUT && inputs == 0 && !_showUnusedLogic && _lutNotation != RawLUTs) {
            builder.addBuffer(CircuitBuilder::Right, 3, 0);
            builder.addLabel(CircuitBuilder::Left, 3, 0, (lutData & 1) ? "1" : "0");
            lutO = builder.addPin(CircuitBuilder::Right, 3, 0);
        } else if(hasLUT) {
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
        // lutff_N (flip-flop)
        QPointF ffD, ffEN, ffCLK, ffQ, ffSR;
        builder.setOrigin(18, lcOff);
        if(hasDFF) {
            if(!asyncSR && (d_lutff_global_s_r != -1 || _showUnusedLogic)) {
                builder.addBlock(0, 0, 3, 4);
            } else {
                builder.addBlock(0, 0, 3, 3);
            }
            ffD = builder.addPin(CircuitBuilder::Left, 0, 0, "D");
            ffQ = builder.addPin(CircuitBuilder::Right, 2, 0, "Q");
            if(d_lutff_global_clk != -1 || _showUnusedLogic) {
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
            builder.addBuffer(CircuitBuilder::Right, 0, 0);
            ffD = builder.addPin(CircuitBuilder::Left, 0, 0);
            ffQ = builder.addPin(CircuitBuilder::Right, 0, 0);
        }
        builder.build(lutff + "/ff");

        builder.setColor(NET_COLOR);
        builder.setOrigin(0, lcOff);
        // lutff_N/in_0
        if(hasA) {
            QPointF lutffI0 = builder.moveTo(0, 0);
            if(hasLUT) {
                builder.wireTo(lutI0);
            }
            builder.build(lutff_in0, n_lutff_in0);
        }
        // lutff_N/in_1
        if(hasB) {
            QPointF lutffI1 = builder.moveTo(0, 1);
            if(hasLUT) {
                builder.wireTo(lutI1);
            }
            if(hasCarry) {
                builder.joinTo(QPointF(carryI2.x(), lutffI1.y()), hasLUT);
                builder.wireTo(carryI2);
            }
            builder.build(lutff_in1, n_lutff_in1);
        }
        // lutff_N/in_2
        if(hasC) {
            builder.moveTo(lutI2);
            if(n_lutff_in2 == n_lutff_lin) {
                builder.wireTo(3, 2);
            } else if(hasLUT) {
                QPointF lutffI2 = builder.wireTo(0, 2);
            }
            if(hasCarry) {
                builder.moveTo(carryI0);
                builder.wireTo(1, 2);
                if(n_lutff_in2 == n_lutff_lin) {
                    builder.wireTo(3, 2);
                    builder.junction(hasLUT);
                } else if(hasLUT) {
                    builder.junction();
                } else {
                    QPointF lutffI2 = builder.wireTo(0, 2);
                }
            }
            builder.build(lutff_in2, n_lutff_in2);
        }
        // lutff_N/in_3
        if(hasD) {
            if(d_lutff_in3 == n_lutff_cin) {
                if(hasLUT && hasCarry) {
                    builder.junctionTo(QPointF(carryIn.x(), lutI3.y()));
                } else {
                    builder.moveTo(QPointF(carryIn.x(), lutI3.y()));
                }
            } else {
                QPointF lutffI3 = builder.moveTo(0, 3);
            }
            if(hasLUT) {
                builder.wireTo(lutI3);
            }
            builder.build(lutff_in3, n_lutff_in3);
        }
        // lutff_N/lout
        if(hasLUT) {
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
        // lutff_N/out
        if(l_lutff_out) {
            builder.moveTo(ffQ);
            builder.wireTo(22, 0);
            builder.build(lutff_out, n_lutff_out);
        }
        // carry_mux_in or lutff_N-1/cout
        if(hasCarryIn) {
            builder.moveTo(carryIn);
            if(hasCarry) {
                builder.wireTo(carryI1);
            } else if(hasLUT) {
                builder.wireTo(QPointF(carryIn.x(), lutI3.y()));
            }
            builder.build(lutff_cin, n_lutff_cin);
        }
        hasCarryIn = cfgCarry;
        carryIn    = carryO;
    }

    if(!ffCLKs.isEmpty()) {
        builder.setOrigin(15, -5);

        QPointF tileCLK = builder.moveTo(1, 0);
        builder.wireTo(1, 8 * 6 + 3);
        for(QPointF lcCLK : ffCLKs) {
            builder.junctionTo(QPointF(tileCLK.x(), lcCLK.y()));
            builder.wireTo(lcCLK);
        }
        builder.build(lutff_global_clk, n_lutff_global_clk);
    }

    if(!ffENs.isEmpty()) {
        builder.setOrigin(14, -4);

        if(d_lutff_global_cen == -1) {
            builder.setColor(BLOCK_COLOR);
            builder.addBuffer(CircuitBuilder::Right, 0, 0);
            builder.addLabel(CircuitBuilder::Left, 0, 0, "1");
            builder.addPin(CircuitBuilder::Right, 0, 0);
            builder.build();
        }

        builder.setColor(NET_COLOR);
        QPointF tileEN = builder.moveTo(1, 0);
        builder.wireTo(1, 8 * 6 + 2);
        for(QPointF lcEN : ffENs) {
            builder.junctionTo(QPointF(tileEN.x(), lcEN.y()));
            builder.wireTo(lcEN);
        }
        builder.build(lutff_global_cen, n_lutff_global_cen);
    }

    if(!ffSRs.isEmpty()) {
        builder.setOrigin(13, -3);

        if(d_lutff_global_s_r == -1) {
            builder.setColor(BLOCK_COLOR);
            builder.addBuffer(CircuitBuilder::Right, 0, 0);
            builder.addLabel(CircuitBuilder::Left, 0, 0, "0");
            builder.addPin(CircuitBuilder::Right, 0, 0);
            builder.build();
        }

        builder.setColor(NET_COLOR);
        QPointF tileRS = builder.moveTo(1, 0);
        builder.wireTo(1, 8 * 6 + 1);
        for(QPointF lcSR : ffSRs) {
            builder.junctionTo(QPointF(tileRS.x(), lcSR.y()));
            builder.wireTo(lcSR);
        }
        builder.build(lutff_global_s_r, n_lutff_global_s_r);
    }

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
