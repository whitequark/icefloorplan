#ifndef FLOORPLANBUILDER_H
#define FLOORPLANBUILDER_H

#include "bitstream.h"
#include "chipdb.h"

class QGraphicsScene;
class QGraphicsRectItem;

class FloorplanBuilder
{
public:
    enum LUTNotation { VerboseLUTs, CompactLUTs, RawLUTs };

    FloorplanBuilder(ChipDB *chipDB, Bitstream *bitstream, QGraphicsScene *scene,
                     LUTNotation lutNotation = RawLUTs, bool showUnusedLogic = false);

    void buildTiles();
    void buildTile(const Bitstream::Tile &tile);
    void buildLogicTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem);
    void buildIOTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem);
    void buildRAMTile(const Bitstream::Tile &tile, QGraphicsRectItem *tileItem);

private:
    LUTNotation _lutNotation;
    bool _showUnusedLogic;

    ChipDB *_chip;
    Bitstream *_bitstream;
    QGraphicsScene *_scene;

    QString recognizeFunction(uint lutData, bool hasA, bool hasB, bool hasC, bool hasD,
                              bool describeInputs = true) const;
};

#endif // FLOORPLANBUILDER_H
