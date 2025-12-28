#include "mainwindow.h"
#include "filescanner.h"
#include "config.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <random>
#include <algorithm>

MainWindow::MainWindow(const QString &sourceDir, QWidget *parent)
    : QMainWindow(parent)
    , m_sourceDir(sourceDir)
{
    Config &cfg = Config::instance();
    m_currentVolume = cfg.defaultVolume();

    setWindowTitle(QString("Goobert %1").arg(GOOBERT_VERSION));
    resize(1500, 900);

    setupUi();
}

MainWindow::~MainWindow()
{
    stopGrid();
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Splitter: wall on top, controls on bottom
    m_splitter = new QSplitter(Qt::Vertical, m_centralWidget);
    mainLayout->addWidget(m_splitter);

    // Wall container
    m_wallContainer = new QWidget(m_splitter);
    m_wallContainer->setStyleSheet("background-color: #0a0a0a;");
    m_wallContainer->setToolTip(
        "Keyboard Shortcuts:\n"
        "\n"
        "Global:\n"
        "  Space/P - Pause/Play all\n"
        "  ↑/↓ - Volume up/down (all)\n"
        "  N - Next all\n"
        "  S - Shuffle all\n"
        "  X - Shuffle then next\n"
        "  F11 - Toggle fullscreen\n"
        "\n"
        "Selected Cell:\n"
        "  F - Fullscreen selected\n"
        "  ←/→ - Seek -5s/+5s\n"
        "  L - Toggle loop\n"
        "  Z - Zoom in\n"
        "  Shift+Z - Zoom out\n"
        "  Ctrl+R - Rotate\n"
        "\n"
        "Mouse:\n"
        "  Right Click - Pause cell\n"
        "  Double Click - Fullscreen cell\n"
        "  Middle Click - Toggle loop\n"
        "  Scroll Wheel - Frame step\n"
        "  Side Scroll - Seek ±30s"
    );
    m_gridLayout = new QGridLayout(m_wallContainer);
    m_gridLayout->setContentsMargins(2, 2, 2, 2);
    m_gridLayout->setSpacing(2);

    // Control panel
    m_controlPanel = new ControlPanel(m_sourceDir, m_splitter);
    m_controlPanel->installEventFilter(this);  // Capture keyboard events

    m_splitter->addWidget(m_wallContainer);
    m_splitter->addWidget(m_controlPanel);

    // Set stretch factors: video wall gets 9x more space than controls
    m_splitter->setStretchFactor(0, 9);  // Wall
    m_splitter->setStretchFactor(1, 1);  // Controls

    // Initial sizes: give wall 90% of space
    m_splitter->setSizes({900, 100});

    // Connect control panel signals
    connect(m_controlPanel, &ControlPanel::startClicked, this, &MainWindow::startGrid);
    connect(m_controlPanel, &ControlPanel::stopClicked, this, &MainWindow::stopGrid);
    connect(m_controlPanel, &ControlPanel::fullscreenClicked, this, &MainWindow::toggleFullscreen);
    connect(m_controlPanel, &ControlPanel::playPauseClicked, this, &MainWindow::playPauseAll);
    connect(m_controlPanel, &ControlPanel::nextClicked, this, &MainWindow::nextAll);
    connect(m_controlPanel, &ControlPanel::prevClicked, this, &MainWindow::prevAll);
    connect(m_controlPanel, &ControlPanel::shuffleClicked, this, &MainWindow::shuffleAll);
    connect(m_controlPanel, &ControlPanel::muteClicked, this, &MainWindow::muteAll);
    connect(m_controlPanel, &ControlPanel::volumeChanged, this, &MainWindow::setVolumeAll);
    connect(m_controlPanel, &ControlPanel::fileRenamed, this, &MainWindow::onFileRenamed);
    connect(m_controlPanel, &ControlPanel::gridSizeChanged, this, [this](int rows, int cols) {
        m_rows = rows;
        m_cols = cols;
    });
}

void MainWindow::startGrid()
{
    m_sourceDir = m_controlPanel->sourceDir();
    m_rows = m_controlPanel->rows();
    m_cols = m_controlPanel->cols();

    // Scan for media files
    FileScanner scanner;
    QStringList files = scanner.scan(m_sourceDir);

    if (files.isEmpty()) {
        QMessageBox::warning(this, "No Media", "No media files found in " + m_sourceDir);
        return;
    }

    m_controlPanel->log(QString("Found %1 files").arg(files.size()));

    clearGrid();
    buildGrid(m_rows, m_cols);

    // Distribute files to cells
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            GridCell *cell = m_cellMap[{r, c}];
            if (cell) {
                // Shuffle files for each cell
                QStringList shuffled = files;
                std::random_device rd;
                std::mt19937 gen(rd());
                std::shuffle(shuffled.begin(), shuffled.end(), gen);
                cell->setPlaylist(shuffled);
                cell->play();
            }
        }
    }

    m_controlPanel->setRunning(true);
    m_controlPanel->log(QString("Started %1x%2 grid").arg(m_cols).arg(m_rows));
}

void MainWindow::stopGrid()
{
    for (GridCell *cell : m_cells) {
        cell->stop();
    }
    clearGrid();
    m_controlPanel->setRunning(false);
    m_controlPanel->log("Stopped");
}

void MainWindow::buildGrid(int rows, int cols)
{
    for (int r = 0; r < rows; ++r) {
        m_gridLayout->setRowStretch(r, 1);
        for (int c = 0; c < cols; ++c) {
            m_gridLayout->setColumnStretch(c, 1);

            auto *cell = new GridCell(r, c, m_wallContainer);
            m_gridLayout->addWidget(cell, r, c);
            m_cells.append(cell);
            m_cellMap[{r, c}] = cell;

            connect(cell, &GridCell::selected, this, &MainWindow::onCellSelected);
            connect(cell, &GridCell::doubleClicked, this, &MainWindow::onCellDoubleClicked);
            connect(cell, &GridCell::fileChanged, m_controlPanel, &ControlPanel::updateCellStatus);
        }
    }
}

void MainWindow::clearGrid()
{
    for (GridCell *cell : m_cells) {
        m_gridLayout->removeWidget(cell);
        delete cell;
    }
    m_cells.clear();
    m_cellMap.clear();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Intercept key presses for arrow keys, p, f from any child widget
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        // Only handle specific keys that we want to intercept
        if (key == Qt::Key_Up || key == Qt::Key_Down ||
            key == Qt::Key_Left || key == Qt::Key_Right ||
            key == Qt::Key_P || key == Qt::Key_F) {

            // Forward to our keyPressEvent
            keyPressEvent(keyEvent);
            return true;  // Event handled
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    Qt::KeyboardModifiers mods = event->modifiers();

    // Handle lowercase letters specifically
    QString text = event->text().toLower();

    switch (event->key()) {
    case Qt::Key_F11:
        toggleFullscreen();
        break;
    case Qt::Key_Escape:
        exitFullscreen();
        break;

    // Grid controls
    case Qt::Key_N:
        nextAllIfNotLooping();
        break;
    case Qt::Key_S:
        shuffleAll();
        break;
    case Qt::Key_X:
        shuffleThenNextAll();
        break;
    case Qt::Key_Space:
    case Qt::Key_P:
        playPauseAll();
        break;

    // Volume controls (all cells)
    case Qt::Key_Up:
        volumeUpAll();
        event->accept();
        break;
    case Qt::Key_Down:
        volumeDownAll();
        event->accept();
        break;

    // Fullscreen selected cell
    case Qt::Key_F:
        if (text == "f") {  // Only lowercase f
            toggleTileFullscreen();
        } else {
            QMainWindow::keyPressEvent(event);
        }
        break;

    // Selected cell controls
    case Qt::Key_R:
        if (mods & Qt::ControlModifier) {
            rotateSelected();
        }
        break;
    case Qt::Key_Z:
        if (mods & Qt::ShiftModifier) {
            zoomOutSelected();
        } else {
            zoomInSelected();
        }
        break;
    case Qt::Key_L:
        toggleLoopSelected();
        break;

    // Seek (selected cell)
    case Qt::Key_Left:
        seekSelected(-5);
        event->accept();
        break;
    case Qt::Key_Right:
        seekSelected(5);
        event->accept();
        break;

    default:
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::toggleFullscreen()
{
    if (m_isFullscreen) {
        exitFullscreen();
    } else {
        m_isFullscreen = true;
        m_controlPanel->hide();
        showFullScreen();
        m_controlPanel->log("Fullscreen ON");
    }
}

void MainWindow::exitFullscreen()
{
    if (m_isTileFullscreen) {
        exitTileFullscreen();
    }
    if (m_isFullscreen) {
        m_isFullscreen = false;
        showNormal();
        m_controlPanel->show();
        m_controlPanel->log("Fullscreen OFF");
    }
}

void MainWindow::enterTileFullscreen(int row, int col)
{
    GridCell *cell = m_cellMap.value({row, col});
    if (!cell) return;

    // Hide all other cells
    for (GridCell *c : m_cells) {
        if (c != cell) {
            c->hide();
            c->pause();
            c->mute();
        }
    }

    // Make selected cell fill the grid
    m_gridLayout->removeWidget(cell);
    m_gridLayout->addWidget(cell, 0, 0, m_rows, m_cols);

    m_isTileFullscreen = true;
    m_fullscreenCell = cell;

    if (!m_isFullscreen) {
        toggleFullscreen();
    }

    m_controlPanel->log(QString("Tile fullscreen: [%1,%2]").arg(row).arg(col));
}

void MainWindow::exitTileFullscreen()
{
    if (!m_isTileFullscreen) return;

    // Restore all cells
    for (GridCell *c : m_cells) {
        m_gridLayout->removeWidget(c);
    }

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            GridCell *cell = m_cellMap.value({r, c});
            if (cell) {
                m_gridLayout->addWidget(cell, r, c);
                cell->show();
                cell->unmute();
                cell->play();
            }
        }
    }

    m_isTileFullscreen = false;
    m_fullscreenCell = nullptr;
    m_controlPanel->log("Tile fullscreen OFF");
}

void MainWindow::onCellSelected(int row, int col)
{
    m_selectedRow = row;
    m_selectedCol = col;

    GridCell *cell = m_cellMap.value({row, col});
    if (cell) {
        m_controlPanel->setSelectedPath(cell->currentFile());
    }
}

void MainWindow::onCellDoubleClicked(int row, int col)
{
    if (m_isTileFullscreen) {
        exitTileFullscreen();
    } else {
        enterTileFullscreen(row, col);
    }
}

void MainWindow::playPauseAll()
{
    for (GridCell *cell : m_cells) {
        cell->togglePause();
    }
}

void MainWindow::nextAll()
{
    for (GridCell *cell : m_cells) {
        cell->next();
    }
}

void MainWindow::prevAll()
{
    for (GridCell *cell : m_cells) {
        cell->prev();
    }
}

void MainWindow::shuffleAll()
{
    for (GridCell *cell : m_cells) {
        cell->shuffle();
    }
}

void MainWindow::muteAll()
{
    for (GridCell *cell : m_cells) {
        cell->toggleMute();
    }
}

void MainWindow::setVolumeAll(int volume)
{
    m_currentVolume = volume;
    for (GridCell *cell : m_cells) {
        cell->setVolume(volume);
    }
}

void MainWindow::volumeUpAll()
{
    m_currentVolume = qMin(100, m_currentVolume + 5);
    setVolumeAll(m_currentVolume);
    m_controlPanel->log(QString("Volume: %1%").arg(m_currentVolume));
}

void MainWindow::volumeDownAll()
{
    m_currentVolume = qMax(0, m_currentVolume - 5);
    setVolumeAll(m_currentVolume);
    m_controlPanel->log(QString("Volume: %1%").arg(m_currentVolume));
}

void MainWindow::toggleTileFullscreen()
{
    if (m_isTileFullscreen) {
        exitTileFullscreen();
    } else {
        // Fullscreen the selected cell
        if (m_selectedRow >= 0 && m_selectedCol >= 0) {
            enterTileFullscreen(m_selectedRow, m_selectedCol);
        }
    }
}

void MainWindow::nextAllIfNotLooping()
{
    for (GridCell *cell : m_cells) {
        cell->nextIfNotLooping();
    }
}

void MainWindow::shuffleThenNextAll()
{
    // First shuffle all
    shuffleAll();

    // Then next after delay (200ms)
    QTimer::singleShot(200, this, &MainWindow::nextAllIfNotLooping);
}

GridCell* MainWindow::selectedCell() const
{
    return m_cellMap.value({m_selectedRow, m_selectedCol}, nullptr);
}

void MainWindow::toggleLoopSelected()
{
    if (GridCell *cell = selectedCell()) {
        cell->toggleLoopFile();
    }
}

void MainWindow::frameStepSelected()
{
    if (GridCell *cell = selectedCell()) {
        cell->frameStep();
    }
}

void MainWindow::frameBackStepSelected()
{
    if (GridCell *cell = selectedCell()) {
        cell->frameBackStep();
    }
}

void MainWindow::rotateSelected()
{
    if (GridCell *cell = selectedCell()) {
        cell->rotateVideo();
    }
}

void MainWindow::zoomInSelected()
{
    if (GridCell *cell = selectedCell()) {
        cell->zoomIn();
    }
}

void MainWindow::zoomOutSelected()
{
    if (GridCell *cell = selectedCell()) {
        cell->zoomOut();
    }
}

void MainWindow::seekSelected(double seconds)
{
    if (GridCell *cell = selectedCell()) {
        cell->seekRelative(seconds);
    }
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    int hdelta = event->angleDelta().x();
    int vdelta = event->angleDelta().y();

    Config &cfg = Config::instance();
    int seekAmount = cfg.seekAmountSeconds();

    // Horizontal scroll (side scroll on MX Master): seek by configured amount
    if (hdelta != 0) {
        if (hdelta < 0) {
            seekSelected(-seekAmount);
        } else {
            seekSelected(seekAmount);
        }
    }
    // Vertical scroll: frame step
    else if (vdelta != 0) {
        if (vdelta < 0) {
            frameStepSelected();
        } else {
            frameBackStepSelected();
        }
    }

    event->accept();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        // MBTN_MID: toggle loop on selected cell
        toggleLoopSelected();
        event->accept();
    } else if (event->button() == Qt::BackButton) {
        // MBTN_BACK: chained shuffle + next
        shuffleThenNextAll();
        event->accept();
    } else {
        QMainWindow::mousePressEvent(event);
    }
}

void MainWindow::onFileRenamed(const QString &oldPath, const QString &newPath)
{
    // Update all cells' playlists
    for (GridCell *cell : m_cells) {
        cell->updatePlaylistPath(oldPath, newPath);
    }
}
