#include <QProgressBar>
#include <QMessageBox>
#include "chipdbloader.h"
#include "bitstreamloader.h"
#include "floorplanwindow.h"
#include "ui_floorplanwindow.h"

FloorplanWindow::FloorplanWindow(QWidget *parent) :
    QMainWindow(parent), _ui(new Ui::FloorplanWindow)
{
    _ui->setupUi(this);
    _ui->statusBar->addPermanentWidget(&_progressBar);
    _progressBar.hide();
}

FloorplanWindow::~FloorplanWindow()
{
    delete _ui;
}

void FloorplanWindow::openFile()
{
    loadBitstream(":/examples/blinky.txt");
}

void FloorplanWindow::loadBitstream(QString filename)
{
    _ui->statusBar->showMessage("Loading bitstream " + filename + "...");
    _progressBar.show();

    BitstreamLoader *bitstreamLoader = new BitstreamLoader(this, filename);
    connect(bitstreamLoader, &QThread::finished, bitstreamLoader, &QObject::deleteLater);

    connect(bitstreamLoader, &BitstreamLoader::progress, this, [=](int cur, int max) {
        _progressBar.setRange(0, max);
        _progressBar.setValue(cur);
    });
    connect(bitstreamLoader, &BitstreamLoader::ready, this, [=](Bitstream bitstream) {
        _bitstream = bitstream;
        loadChipDB(_bitstream.device);
    });
    connect(bitstreamLoader, &BitstreamLoader::failed, this, [=] {
        _progressBar.hide();
        _ui->statusBar->clearMessage();
        QMessageBox::critical(this, "Error", "Cannot parse bitstream " + filename + "!");
    });

    bitstreamLoader->start();
}

void FloorplanWindow::loadChipDB(QString device)
{
    if(_chipDBCache.contains(device)) {
        updateFloorplan();
        return;
    }

    _ui->statusBar->showMessage("Loading chipdb for " + device + "...");
    _progressBar.show();

    ChipDBLoader *chipDBLoader = new ChipDBLoader(this, device);
    connect(chipDBLoader, &QThread::finished, chipDBLoader, &QObject::deleteLater);

    connect(chipDBLoader, &ChipDBLoader::progress, this, [=](int cur, int max) {
        _progressBar.setRange(0, max);
        _progressBar.setValue(cur);
    });
    connect(chipDBLoader, &ChipDBLoader::ready, this, [=](ChipDB chipDB) {
        _chipDBCache.insert(device, chipDB);
        updateFloorplan();
    });
    connect(chipDBLoader, &ChipDBLoader::failed, this, [=] {
        _progressBar.hide();
        _ui->statusBar->clearMessage();
        QMessageBox::critical(this, "Error", "Cannot parse chipdb for " + device + "!");
    });

    chipDBLoader->start();
}

void FloorplanWindow::updateFloorplan()
{
    _progressBar.hide();
    if(_bitstream.validate(_chipDBCache[_bitstream.device])) {
        _ui->statusBar->showMessage("Ready.");
        _ui->floorplan->setData(&_bitstream, &_chipDBCache[_bitstream.device]);
    } else {
        _ui->statusBar->clearMessage();
        QMessageBox::critical(this, "Error", "Cannot validate bitstream produced by " +
                                             _bitstream.comment + "!");
    }
}
