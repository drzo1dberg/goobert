#include "mainwindow.h"
#include "filescanner.h"
#include "config.h"
#include "keymap.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <random>
#include <algorithm>
#include <utility>

MainWindow::MainWindow(QString sourceDir, QWidget *parent)
    : QMainWindow(parent)
    , m_sourceDir(std::move(sourceDir))
{
    using namespace MainWindowConstants;
    Config &cfg = Config::instance();
    m_currentVolume = cfg.defaultVolume();

    setWindowTitle(QString("Goobert %1").arg(GOOBERT_VERSION));
    resize(kDefaultWidth, kDefaultHeight);

    setupUi();
}

MainWindow::~MainWindow()
{
    stopGrid();
}

void MainWindow::setupUi()
{
    using namespace MainWindowConstants;

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
    m_gridLayout = new QGridLayout(m_wallContainer);
    m_gridLayout->setContentsMargins(kGridMargin, kGridMargin, kGridMargin, kGridMargin);
    m_gridLayout->setSpacing(kGridSpacing);

    // Control panel
    m_controlPanel = new ControlPanel(m_sourceDir, m_splitter);
    m_controlPanel->installEventFilter(this);  // Capture keyboard events
    // Set tooltip from KeyMap (automatically shows all bindings including duplicates)
    m_controlPanel->setToolTip(KeyMap::instance().generateTooltip());

    m_splitter->addWidget(m_wallContainer);
    m_splitter->addWidget(m_controlPanel);

    // Set stretch factors: video wall gets 9x more space than controls
    m_splitter->setStretchFactor(0, kWallStretchFactor);
    m_splitter->setStretchFactor(1, kControlStretchFactor);

    // Initial sizes: give wall 90% of space
    m_splitter->setSizes({kInitialWallSize, kInitialControlSize});

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
    connect(m_controlPanel, &ControlPanel::customSourceRequested, this, &MainWindow::onCustomSource);
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
    QString filter = m_controlPanel->filter();

    // Scan for media files with optional filter
    FileScanner scanner;
    QStringList files = scanner.scan(m_sourceDir, filter);

    if (files.isEmpty()) {
        QString msg = filter.isEmpty()
            ? QString("No media files found in %1").arg(m_sourceDir)
            : QString("No files matching filter '%1' in %2").arg(filter, m_sourceDir);
        QMessageBox::warning(this, "No Media", msg);
        return;
    }

    QString logMsg = filter.isEmpty()
        ? QString("Found %1 files").arg(files.size())
        : QString("Found %1 files (filter: %2)").arg(files.size()).arg(filter);
    m_controlPanel->log(logMsg);

    clearGrid();
    buildGrid(m_rows, m_cols);
    m_currentFilter = filter;
    m_cellPlaylists.clear();

    // Distribute files to cells
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            GridCell *cell = m_cellMap[{r, c}];
            if (cell) {
                // Shuffle files for each cell using static RNG
                QStringList shuffled = files;
                std::shuffle(shuffled.begin(), shuffled.end(), s_rng);
                cell->setPlaylist(shuffled);
                cell->play();
                // Store playlist for auto-restart
                m_cellPlaylists[{r, c}] = shuffled;
            }
        }
    }

    m_controlPanel->setRunning(true);
    m_controlPanel->log(QString("Started %1x%2 grid").arg(m_cols).arg(m_rows));

    // Auto-select first cell
    if (!m_cells.isEmpty()) {
        onCellSelected(0, 0);
    }

    // Start watchdog timer
    if (!m_watchdogTimer) {
        m_watchdogTimer = new QTimer(this);
        connect(m_watchdogTimer, &QTimer::timeout, this, &MainWindow::watchdogCheck);
    }
    m_watchdogTimer->start(MainWindowConstants::kWatchdogIntervalMs);
}

void MainWindow::stopGrid()
{
    // Stop watchdog
    if (m_watchdogTimer) {
        m_watchdogTimer->stop();
    }

    for (GridCell *cell : m_cells) {
        cell->stop();
    }
    clearGrid();
    m_cellPlaylists.clear();
    m_selectedCell = nullptr;
    m_selectedRow = -1;
    m_selectedCol = -1;
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

    // Reset all stretch factors
    for (int i = 0; i < MainWindowConstants::kMaxGridSize; ++i) {
        m_gridLayout->setRowStretch(i, 0);
        m_gridLayout->setColumnStretch(i, 0);
    }

    // Clear monitor table
    m_controlPanel->clearMonitor();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Intercept key presses for arrow keys, p, f, space from any child widget
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        // Only handle specific keys that we want to intercept
        if (key == Qt::Key_Up || key == Qt::Key_Down ||
            key == Qt::Key_Left || key == Qt::Key_Right ||
            key == Qt::Key_P || key == Qt::Key_F || key == Qt::Key_Space ||
            key == Qt::Key_V || key == Qt::Key_C ||
            key == Qt::Key_B || key == Qt::Key_N) {

            // Forward to our keyPressEvent
            keyPressEvent(keyEvent);
            return true;  // Event handled
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Get action from keymap
    KeyMap::Action action = KeyMap::instance().getAction(event);

    // Execute action
    switch (action) {
    // Global actions
    case KeyMap::Action::PauseAll:
        playPauseAll();
        break;
    case KeyMap::Action::VolumeUp:
        volumeUpAll();
        break;
    case KeyMap::Action::VolumeDown:
        volumeDownAll();
        break;
    case KeyMap::Action::NextAll:
        nextAll();
        break;
    case KeyMap::Action::ShuffleAll:
        shuffleAll();
        break;
    case KeyMap::Action::ShuffleThenNextAll:
        shuffleThenNextAll();
        break;
    case KeyMap::Action::FullscreenGlobal:
        toggleFullscreen();
        break;
    case KeyMap::Action::ExitFullscreen:
        exitFullscreen();
        break;

    // Navigation
    case KeyMap::Action::NavigateUp:
        navigateSelection(0, -1);
        event->accept();
        break;
    case KeyMap::Action::NavigateDown:
        navigateSelection(0, 1);
        event->accept();
        break;
    case KeyMap::Action::NavigateLeft:
        navigateSelection(-1, 0);
        event->accept();
        break;
    case KeyMap::Action::NavigateRight:
        navigateSelection(1, 0);
        event->accept();
        break;

    // Selected cell actions
    case KeyMap::Action::FullscreenSelected:
        toggleTileFullscreen();
        break;
    case KeyMap::Action::SeekForward:
        seekSelected(MainWindowConstants::kSeekStepSeconds);
        break;
    case KeyMap::Action::SeekBackward:
        seekSelected(-MainWindowConstants::kSeekStepSeconds);
        break;
    case KeyMap::Action::FrameStepForward:
        frameStepSelected();  // N key: one frame forward
        break;
    case KeyMap::Action::FrameStepBackward:
        frameBackStepSelected();  // B key: one frame backward
        break;
    case KeyMap::Action::ToggleLoop:
        toggleLoopSelected();
        break;
    case KeyMap::Action::NextSelected:
        if (m_selectedCell) m_selectedCell->next();
        break;
    case KeyMap::Action::PrevSelected:
        if (m_selectedCell) m_selectedCell->prev();
        break;
    case KeyMap::Action::ZoomIn:
        zoomInSelected();
        break;
    case KeyMap::Action::ZoomOut:
        zoomOutSelected();
        break;
    case KeyMap::Action::Rotate:
        rotateSelected();
        break;
    case KeyMap::Action::Screenshot:
        screenshotSelected();
        break;

    case KeyMap::Action::NoAction:
    default:
        QMainWindow::keyPressEvent(event);
        break;
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

    // Enable full mpv GUI (OSC) for fullscreen cell
    cell->setOscEnabled(true);
    cell->setOsdLevel(1);

    if (!m_isFullscreen) {
        toggleFullscreen();
    }

    m_controlPanel->log(QString("Tile fullscreen: [%1,%2]").arg(row).arg(col));
}

void MainWindow::exitTileFullscreen()
{
    if (!m_isTileFullscreen) return;

    // Disable OSC on fullscreen cell before restoring grid
    if (m_fullscreenCell) {
        m_fullscreenCell->setOscEnabled(false);
        m_fullscreenCell->setOsdLevel(0);
    }

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
    // Deselect previous cell
    if (m_selectedCell) {
        m_selectedCell->setSelected(false);
    }

    m_selectedRow = row;
    m_selectedCol = col;

    GridCell *cell = m_cellMap.value({row, col});
    if (cell) {
        // Select new cell
        cell->setSelected(true);
        m_selectedCell = cell;
        m_controlPanel->setSelectedPath(cell->currentFile());
    }
}

void MainWindow::onCellDoubleClicked(int row, int col)
{
    if (m_isTileFullscreen) {
        exitTileFullscreen();
    } else {
        // Select the cell first (like F does)
        onCellSelected(row, col);
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
        cell->nextIfNotLooping();
    }
}

void MainWindow::prevAll()
{
    for (GridCell *cell : m_cells) {
        cell->prevIfNotLooping();
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
    using namespace MainWindowConstants;
    m_currentVolume = qMin(100, m_currentVolume + kVolumeStep);
    setVolumeAll(m_currentVolume);
    m_controlPanel->log(QString("Volume: %1%").arg(m_currentVolume));
}

void MainWindow::volumeDownAll()
{
    using namespace MainWindowConstants;
    m_currentVolume = qMax(0, m_currentVolume - kVolumeStep);
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

void MainWindow::shuffleThenNextAll()
{
    shuffleAll();
    QTimer::singleShot(MainWindowConstants::kShuffleNextDelayMs, this, &MainWindow::nextAll);
}

GridCell* MainWindow::selectedCell() const noexcept
{
    return m_selectedCell;
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

void MainWindow::screenshotSelected()
{
    if (GridCell *cell = selectedCell()) {
        cell->screenshot();
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
            seekSelected(seekAmount);  // Scroll left -> forward
        } else {
            seekSelected(-seekAmount);  // Scroll right -> backward
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
    if (event->button() == Qt::BackButton) {
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

void MainWindow::onCustomSource(int row, int col, const QStringList &paths)
{
    GridCell *cell = m_cellMap.value({row, col});
    if (!cell) return;

    // Scan for media files
    FileScanner scanner;
    QStringList files;
    for (const QString &path : paths) {
        files.append(scanner.scan(path));
    }

    if (files.isEmpty()) {
        m_controlPanel->log(QString("No media found for [%1,%2]").arg(row).arg(col));
        return;
    }

    // Shuffle files
    std::shuffle(files.begin(), files.end(), s_rng);

    // Set playlist and play
    cell->setPlaylist(files);
    cell->play();
    cell->setVolume(m_currentVolume);

    // Auto loop-inf if single file
    if (files.size() == 1) {
        cell->setLoopFile(true);
        m_controlPanel->log(QString("[%1,%2]: 1 file, loop=inf").arg(row).arg(col));
    } else {
        m_controlPanel->log(QString("[%1,%2]: %3 files").arg(row).arg(col).arg(files.size()));
    }
}

void MainWindow::navigateSelection(int colDelta, int rowDelta)
{
    // Don't navigate if no cells
    if (m_cells.isEmpty()) return;

    // If nothing selected, select first cell
    if (m_selectedRow < 0 || m_selectedCol < 0) {
        onCellSelected(0, 0);
        return;
    }

    // Calculate new position
    int newCol = m_selectedCol + colDelta;
    int newRow = m_selectedRow + rowDelta;

    // Wrap around or clamp
    if (newCol < 0) newCol = m_cols - 1;
    if (newCol >= m_cols) newCol = 0;
    if (newRow < 0) newRow = m_rows - 1;
    if (newRow >= m_rows) newRow = 0;

    // Select new cell
    onCellSelected(newRow, newCol);
}

void MainWindow::watchdogCheck()
{
    // Check each cell and restart if it seems dead (no file playing)
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            GridCell *cell = m_cellMap.value({r, c});
            if (!cell) continue;

            // Skip if cell is in tile fullscreen mode (hidden cells are expected to be idle)
            if (m_isTileFullscreen && cell != m_fullscreenCell) continue;

            // Check if cell has no current file (indicates it may have stopped)
            QString currentFile = cell->currentFile();
            if (currentFile.isEmpty()) {
                // Try to restart with stored playlist
                QStringList playlist = m_cellPlaylists.value({r, c});
                if (!playlist.isEmpty()) {
                    m_controlPanel->log(QString("Restarting cell [%1,%2]").arg(r).arg(c));
                    std::shuffle(playlist.begin(), playlist.end(), s_rng);
                    cell->setPlaylist(playlist);
                    cell->play();
                    cell->setVolume(m_currentVolume);
                }
            }
        }
    }
}
