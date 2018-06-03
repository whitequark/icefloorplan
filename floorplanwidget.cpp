#include <QtDebug>
#include <QOpenGLWidget>
#include <QGraphicsPathItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include "chipdb.h"
#include "bitstream.h"
#include "circuitbuilder.h"
#include "floorplanwidget.h"

static const qreal  GRID = 20;

static const QColor BLOCK_COLOR = Qt::darkRed;
static const QColor NET_COLOR   = Qt::darkGreen;

static const QColor TILE_INACTIVE_COLOR = QColor::fromRgb(0xC0C0C0);
static const QColor TILE_IO_COLOR       = QColor::fromRgb(0xAAEEEE);
static const QColor TILE_LOGIC_COLOR    = QColor::fromRgb(0xFBEAFB);
static const QColor TILE_RAM_COLOR      = QColor::fromRgb(0xEEEEAA);

FloorplanWidget::FloorplanWidget(QWidget *parent)
    : QGraphicsView(parent), _showUnusedLogic(false),
      _bitstream(nullptr), _chip(nullptr), _hovered(nullptr)
{
    setUseOpenGL(false);
    setScene(&_scene);
    _scene.setBackgroundBrush(Qt::white);
}

void FloorplanWidget::clear()
{
    _hovered = nullptr;
    _scene.clear();
}

void FloorplanWidget::resetZoom()
{
    _scene.setSceneRect(_scene.itemsBoundingRect() +
                        QMarginsF(GRID, GRID, GRID, GRID));
    fitInView(_scene.sceneRect(), Qt::KeepAspectRatio);
}

void FloorplanWidget::wheelEvent(QWheelEvent *event)
{
    if(event->modifiers() == Qt::ControlModifier) {
        float s = 1 + event->angleDelta().y() / 1200.0;
        scale(s, s);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void FloorplanWidget::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsPathItem *netItem = nullptr;

    QRect hoverRect = QRect(event->pos(), QSize()).adjusted(-10, -10, 10, 10);
    for(QGraphicsItem *hoverItem : items(hoverRect)) {
        QGraphicsPathItem *item = qgraphicsitem_cast<QGraphicsPathItem *>(hoverItem);
        if(item->data(0).isValid()) {
            netItem = item;
            break;
        }
    }

    if(netItem != _hovered) {
        if(_hovered) {
            _hovered->setPen(_hoveredOldPen);
        }
        if(netItem) {
            _hoveredOldPen = netItem->pen();
            netItem->setPen(QPen(Qt::red));
            _hovered = netItem;
        } else {
            _hovered = nullptr;
        }

        if(_hovered) {
            net_t net = netItem->data(0).toInt();
            QString symbol;
            if(_bitstream->symbols.contains(net)) {
                symbol = _bitstream->symbols[net];
            }
            emit netHovered(net, netItem->toolTip(), symbol);
        } else {
            emit netHovered(-1, QString(), QString());
        }
    }
}

void FloorplanWidget::setUseOpenGL(bool use)
{
    _useOpenGL = use;
    setViewport(_useOpenGL ? new QOpenGLWidget : new QWidget);
    viewport()->setMouseTracking(true);
}

void FloorplanWidget::setShowUnusedLogic(bool show)
{
    _showUnusedLogic = show;
    buildTiles();
}

void FloorplanWidget::setData(Bitstream *bitstream, ChipDB *chip)
{
    _bitstream = bitstream;
    _chip = chip;

    buildTiles();
//    buildTile(bitstream->tile(4, 5));

    resetZoom();
}

void FloorplanWidget::buildTiles()
{
    if(!_bitstream) return;

    clear();
    for(auto &tile : _bitstream->tiles) {
        buildTile(tile);
    }
}

void FloorplanWidget::buildTile(const Bitstream::Tile &tile)
{
    const qreal WIDTH  = 75;
    const qreal HEIGHT = 75;

    QGraphicsRectItem *tileItem =
            _scene.addRect(QRectF(QPointF(-8, -8) * GRID,
                                  QSizeF(WIDTH - 16, HEIGHT - 16) * GRID),
                           Qt::NoPen, Qt::NoBrush);
    tileItem->setPos(QPointF(tile.x * WIDTH, (_chip->height - tile.y) * HEIGHT) * GRID);

    QGraphicsSimpleTextItem *coordsItem =
            _scene.addSimpleText(QString("%3 (%1 %2)").arg(tile.x).arg(tile.y).arg(tile.type),
                                 QFont("sans", 18));
    coordsItem->setParentItem(tileItem);
    coordsItem->setPos(QPointF(-8, -10) * GRID);

    if(tile.type == "logic") {
        buildLogicTile(tile, tileItem);
    }
}

void FloorplanWidget::buildLogicTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem)
{
    CircuitBuilder builder(tileItem);
    builder.setGrid(GRID);

    const auto &tileNets = _chip->tileNets(tile.x, tile.y);
    const auto &tileBits = _chip->tilesBits["logic"];
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
    QString carry_in = "carry_in";
    net_t n_carry_in = tileNets[carry_in];
    net_t d_carry_in = netDrivers[n_carry_in];

    bool hasCarryIn;
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
        carryIn = builder.addPin(CircuitBuilder::Up, 1, 0);
        QPointF fabCarryIn = builder.addPin(CircuitBuilder::Down, 1, 0);
        builder.build("carry_in");

        if(d_carry_in != -1) {
            builder.setColor(NET_COLOR);
            builder.moveTo(fabCarryIn);
            builder.segmentTo(fabCarryIn + QPointF(0, 1));
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
            if(lutConfig & (1 << nbit))
                lutData |= 1 << 15;
        }

        bool cfgCarry = lutConfig & (1 << 8);
        bool hasDFF   = lutConfig & (1 << 9);
        bool srSet    = lutConfig & (1 << 18);
        bool asyncSR  = lutConfig & (1 << 19);

        QString lutff = QString("lutff_%1").arg(lc);
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
        net_t n_lutff_lout = lc == 7 ? -1    : tileNets[lutff_lout];
        bool  l_lutff_lout = lc == 7 ? false : netLoaded[n_lutff_lout];
        QString lutff_out  = lutff + "/out";
        net_t n_lutff_out  = tileNets[lutff_out];
        bool  l_lutff_out  = netLoaded[n_lutff_out];

        uint inputs = 0;
        if(d_lutff_in0 != -1) inputs++;
        if(d_lutff_in1 != -1) inputs++;
        if(d_lutff_in2 != -1) inputs++;
        if(d_lutff_in3 != -1) inputs++;

        bool hasLUT   = _showUnusedLogic || hasDFF || l_lutff_lout || l_lutff_out;
        bool hasCarry = _showUnusedLogic || cfgCarry;
        bool hasOut   = _showUnusedLogic || l_lutff_out;
        isActive |= hasDFF || l_lutff_lout || l_lutff_out;

        builder.setColor(BLOCK_COLOR);
        // lutff_N (carry adder)
        QPointF carryI0, carryI1, carryI2, carryO;
        builder.setOrigin(1, lcOff - 1.5);
        if(hasCarry) {
            builder.addMux(CircuitBuilder::Up, 1.5, 0, 3);
            builder.addLabel(CircuitBuilder::Up, 1, 0, "+₁");
            if(d_lutff_in2 != -1)
                carryI0 = builder.addPin(CircuitBuilder::Down, 0, 0);
            if(hasCarryIn)
                carryI1 = builder.addPin(CircuitBuilder::Down, 1, 0);
            if(d_lutff_in1 != -1)
                carryI2 = builder.addPin(CircuitBuilder::Down, 2, 0);
            carryO      = builder.addPin(CircuitBuilder::Up,   1, 0);
        }
        builder.build(lutff + "/carry");
        // lutff_N (look-up table)
        QPointF lutI0, lutI1, lutI2, lutI3, lutO;
        builder.setOrigin(5, lcOff);
        if(hasLUT && inputs == 0 && !_showUnusedLogic) {
            builder.addBuffer(CircuitBuilder::Right, 3, 0);
            builder.addLabel(CircuitBuilder::Left, 3, 0, (lutData & 1) ? "1" : "0");
            lutO = builder.addPin(CircuitBuilder::Right, 3, 0);
        } else if(hasLUT) {
            builder.addBlock(0, 0, 4, 4);
            if(d_lutff_in0 != -1)
                lutI0 = builder.addPin(CircuitBuilder::Left,  0, 0, "A");
            if(d_lutff_in1 != -1)
                lutI1 = builder.addPin(CircuitBuilder::Left,  0, 1, "B");
            if(d_lutff_in2 != -1)
                lutI2 = builder.addPin(CircuitBuilder::Left,  0, 2, "C");
            if(d_lutff_in3 != -1)
                lutI3 = builder.addPin(CircuitBuilder::Left,  0, 3, "D");
            lutO = builder.addPin(CircuitBuilder::Right, 3, 0, "O");
        }
        builder.build(lutff + "/lut");
        // lutff_N (flip-flop)
        QPointF ffD, ffEN, ffCLK, ffQ, ffSR;
        builder.setOrigin(14, lcOff);
        if(hasDFF) {
            if(!asyncSR && (d_lutff_global_s_r != -1 || _showUnusedLogic)) {
                builder.addBlock(0, 0, 3, 4);
            } else {
                builder.addBlock(0, 0, 3, 3);
            }
            ffD = builder.addPin(CircuitBuilder::Left,  0, 0, "D");
            ffQ = builder.addPin(CircuitBuilder::Right, 2, 0, "Q");
            if(d_lutff_global_clk != -1 || _showUnusedLogic) {
                ffCLK = builder.addPin(CircuitBuilder::Left,  0, 1, ">", negClk);
                ffCLKs.append(ffCLK);
            }
            if(d_lutff_global_cen != -1 || _showUnusedLogic) {
                ffEN  = builder.addPin(CircuitBuilder::Left,  0, 2, "E");
                ffENs.append(ffEN);
            }
            if(d_lutff_global_s_r != -1 || _showUnusedLogic) {
                const char *label = srSet ? "S" : "R";
                if(asyncSR) {
                    ffSR = builder.addPin(CircuitBuilder::Down,  1, 2, label);
                } else {
                    ffSR = builder.addPin(CircuitBuilder::Left,  0, 3, label);
                }
                ffSRs.append(ffSR);
            }
        } else if(hasOut) {
            builder.addBuffer(CircuitBuilder::Right, 0, 0);
            ffD = builder.addPin(CircuitBuilder::Left,  0, 0);
            ffQ = builder.addPin(CircuitBuilder::Right, 0, 0);
        }
        builder.build(lutff + "/ff");

        builder.setColor(NET_COLOR);
        builder.setOrigin(0, lcOff);
        // lutff_N/in_0
        if(d_lutff_in0 != -1) {
            QPointF lutffI0 = builder.moveTo(0, 0);
            if(hasLUT) {
                builder.segmentTo(lutI0);
            }
            builder.build(lutff_in0, n_lutff_in0);
        }
        // lutff_N/in_1
        if(d_lutff_in1 != -1) {
            QPointF lutffI1 = builder.moveTo(0, 1);
            if(hasLUT) {
                builder.segmentTo(lutI1);
            }
            if(hasCarry) {
                builder.joinTo(QPointF(carryI2.x(), lutffI1.y()), hasLUT);
                builder.segmentTo(carryI2);
            }
            builder.build(lutff_in1, n_lutff_in1);
        }
        // lutff_N/in_2
        if(d_lutff_in2 != -1) {
            builder.moveTo(lutI2);
            if(n_lutff_in2 == n_lutff_lin) {
                builder.segmentTo(3, 2);
            } else if(hasLUT) {
                QPointF lutffI2 = builder.segmentTo(0, 2);
            }
            if(hasCarry) {
                builder.moveTo(carryI0);
                builder.segmentTo(1, 2);
                if(n_lutff_in2 == n_lutff_lin) {
                    builder.segmentTo(3, 2);
                    builder.junction(hasLUT);
                } else if(hasLUT) {
                    builder.junction();
                } else {
                    QPointF lutffI2 = builder.segmentTo(0, 2);
                }
            }
            builder.build(lutff_in2, n_lutff_in2);
        }
        // lutff_N/in_3
        if(d_lutff_in3 != -1) {
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
                builder.segmentTo(lutI3);
            }
            builder.build(lutff_in3, n_lutff_in3);
        }
        // lutff_N/lout
        if(hasLUT) {
            builder.moveTo(lutO);
            builder.segmentTo(ffD);
            if(l_lutff_lout) {
                builder.junctionTo(9.5, 0);
                builder.segmentTo(9.5, -2);
                builder.segmentTo(3, -2);
                builder.segmentTo(3, -4);
            }
            builder.build(lutff_lout, n_lutff_lout);
        }
        // lutff_N/out
        if(l_lutff_out) {
            builder.moveTo(ffQ);
            builder.segmentTo(18, 0);
            builder.build(lutff_out, n_lutff_out);
        }
        // carry_mux_in or lutff_N-1/cout
        if(hasCarryIn) {
            builder.moveTo(carryIn);
            if(hasCarry) {
                builder.segmentTo(carryI1);
            } else if(hasLUT) {
                builder.segmentTo(QPointF(carryIn.x(), lutI3.y()));
            }
            builder.build(lutff_cin, n_lutff_cin);
        }
        hasCarryIn = hasCarry;
        carryIn = carryO;
    }

    if(!ffCLKs.isEmpty()) {
        builder.setOrigin(0, -5);
        QPointF tileCLK = builder.moveTo(12, 0);
        builder.segmentTo(12, 8 * 6 + 3);
        for(QPointF lcCLK : ffCLKs) {
            builder.junctionTo(QPointF(tileCLK.x(), lcCLK.y()));
            builder.segmentTo(lcCLK);
        }
        builder.build(lutff_global_clk, n_lutff_global_clk);
    }

    if(!ffENs.isEmpty()) {
        if(d_lutff_global_cen == -1) {
            builder.setColor(BLOCK_COLOR);
            builder.setOrigin(10, -4);
            builder.addBuffer(CircuitBuilder::Right, 0, 0);
            builder.addLabel(CircuitBuilder::Left, 0, 0, "1");
            builder.addPin(CircuitBuilder::Right, 0, 0);
            builder.build();
        }

        builder.setColor(NET_COLOR);
        builder.setOrigin(0, -4);
        QPointF tileEN = builder.moveTo(11, 0);
        builder.segmentTo(11, 8 * 6 + 2);
        for(QPointF lcEN : ffENs) {
            builder.junctionTo(QPointF(tileEN.x(), lcEN.y()));
            builder.segmentTo(lcEN);
        }
        builder.build(lutff_global_cen, n_lutff_global_cen);
    }

    if(!ffSRs.isEmpty()) {
        if(d_lutff_global_s_r == -1) {
            builder.setColor(BLOCK_COLOR);
            builder.setOrigin(9, -3);
            builder.addBuffer(CircuitBuilder::Right, 0, 0);
            builder.addLabel(CircuitBuilder::Left, 0, 0, "0");
            builder.addPin(CircuitBuilder::Right, 0, 0);
            builder.build();
        }

        builder.setColor(NET_COLOR);
        builder.setOrigin(0, -3);
        QPointF tileRS = builder.moveTo(10, 0);
        builder.segmentTo(10, 8 * 6 + 1);
        for(QPointF lcSR : ffSRs) {
            builder.junctionTo(QPointF(tileRS.x(), lcSR.y()));
            builder.segmentTo(lcSR);
        }
        builder.build(lutff_global_s_r, n_lutff_global_s_r);
    }

    tileItem->setBrush(QBrush(isActive ? TILE_LOGIC_COLOR : TILE_INACTIVE_COLOR));
}
