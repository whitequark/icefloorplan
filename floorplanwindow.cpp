#include <QFile>
#include <QProgressBar>
#include <QMessageBox>
#include "floorplanwindow.h"
#include "ui_floorplanwindow.h"

ChipDBThread::ChipDBThread(QObject *parent, QString name)
    : QThread(parent), _name(name)
{}

void ChipDBThread::run() {
    QFile f(":/chipdb-" + _name + ".txt");
    f.open(QIODevice::ReadOnly | QIODevice::Text);

    ChipDB db;
    db.name = _name;
    if(db.parse(&f, [=](int cur, int max) {
        emit progress(cur, max);
    })) {
        emit ready(db);
    } else {
        emit failed();
    }
}

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
    loadChipDB("384");
}

void FloorplanWindow::loadChipDB(QString name)
{
    if(_chipDBCache.contains(name)) {
        _ui->floorplan->setChip(&_chipDBCache[name]);
        return;
    }

    _ui->statusBar->showMessage("Loading chipdb for " + name + "...");
    _progressBar.show();

    ChipDBThread *chipDBThread = new ChipDBThread(this, name);
    connect(chipDBThread, &ChipDBThread::finished, chipDBThread, &QObject::deleteLater);
    connect(chipDBThread, &ChipDBThread::finished, &_progressBar, &QWidget::hide);

    connect(chipDBThread, &ChipDBThread::progress, this, [=](int cur, int max) {
        _progressBar.setRange(0, max);
        _progressBar.setValue(cur);
    });
    connect(chipDBThread, &ChipDBThread::ready, this, [=](ChipDB chipDB) {
        _chipDBCache.insert(name, chipDB);
        _ui->floorplan->setChip(&_chipDBCache[name]);
        _ui->statusBar->showMessage("Ready.");
    });
    connect(chipDBThread, &ChipDBThread::failed, this, [=] {
        QMessageBox::critical(this, "Error", "Cannot parse chipdb for " + name + "!");
        _ui->statusBar->clearMessage();
    });

    chipDBThread->start();
}
