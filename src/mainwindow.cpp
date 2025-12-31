#include "mainwindow.h"
#include "filescanner.h"
#include "config.h"
#include "keymap.h"
#include "playlistpicker.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QDateTime>
#include <QStatusBar>
#include <QFileInfo>
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

    // Main window styling with global tooltip fix
    setStyleSheet(R"(
        QMainWindow { background-color: #1a1a1a; }
        QToolTip {
            background-color: #2a2a2a;
            color: #ddd;
            border: 1px solid #555;
            padding: 6px;
            font-size: 12px;
        }
    )");

    // Toolbar at top
    m_toolBar = new ToolBar(this);
    m_toolBar->installEventFilter(this);
    m_toolBar->setToolTip(KeyMap::instance().generateTooltip());
    addToolBar(Qt::TopToolBarArea, m_toolBar);

    // Central widget with horizontal splitter
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Config panel (collapsible, above splitter)
    m_configPanel = new ConfigPanel(m_sourceDir);
    m_configPanel->installEventFilter(this);
    mainLayout->addWidget(m_configPanel);

    // Horizontal splitter: side panel | video wall
    m_splitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    m_splitter->setStyleSheet("QSplitter::handle { background-color: #333; }");
    mainLayout->addWidget(m_splitter, 1);  // stretch factor 1

    // Side panel (left)
    m_sidePanel = new SidePanel();
    m_sidePanel->setMinimumWidth(250);
    m_sidePanel->setMaximumWidth(400);
    m_splitter->addWidget(m_sidePanel);

    // Wall container (right, main area)
    m_wallContainer = new QWidget(m_splitter);
    m_wallContainer->setStyleSheet("background-color: #0a0a0a;");
    m_gridLayout = new QGridLayout(m_wallContainer);
    m_gridLayout->setContentsMargins(kGridMargin, kGridMargin, kGridMargin, kGridMargin);
    m_gridLayout->setSpacing(kGridSpacing);
    m_splitter->addWidget(m_wallContainer);

    // Set stretch factors: side panel fixed, wall expands
    m_splitter->setStretchFactor(0, 0);  // Side panel doesn't stretch
    m_splitter->setStretchFactor(1, 1);  // Wall stretches
    m_splitter->setSizes({280, 1000});

    // Status bar
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: #666; padding: 4px;");
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->setStyleSheet("QStatusBar { background-color: #1a1a1a; border-top: 1px solid #333; }");

    // Connect toolbar signals
    connect(m_toolBar, &ToolBar::startClicked, this, &MainWindow::startGrid);
    connect(m_toolBar, &ToolBar::stopClicked, this, &MainWindow::stopGrid);
    connect(m_toolBar, &ToolBar::fullscreenClicked, this, &MainWindow::toggleFullscreen);
    connect(m_toolBar, &ToolBar::playPauseClicked, this, &MainWindow::playPauseAll);
    connect(m_toolBar, &ToolBar::nextClicked, this, &MainWindow::nextAll);
    connect(m_toolBar, &ToolBar::prevClicked, this, &MainWindow::prevAll);
    connect(m_toolBar, &ToolBar::shuffleClicked, this, &MainWindow::shuffleAll);
    connect(m_toolBar, &ToolBar::muteClicked, this, &MainWindow::muteAll);
    connect(m_toolBar, &ToolBar::volumeChanged, this, &MainWindow::setVolumeAll);

    // Connect config panel signals
    connect(m_configPanel, &ConfigPanel::gridSizeChanged, this, [this](int rows, int cols) {
        m_rows = rows;
        m_cols = cols;
    });

    // Connect side panel signals
    connect(m_sidePanel, &SidePanel::cellSelected, this, &MainWindow::onCellSelected);
    connect(m_sidePanel, &SidePanel::fileRenamed, this, &MainWindow::onFileRenamed);
    connect(m_sidePanel, &SidePanel::customSourceRequested, this, &MainWindow::onCustomSource);
    connect(m_sidePanel, &SidePanel::fileSelected, this, [this](int row, int col, const QString &file) {
        GridCell *cell = m_cellMap.value({row, col});
        if (cell) {
            // Load the specific file and play
            cell->loadFile(file);
            log(QString("Playing %1 in [%2,%3]").arg(QFileInfo(file).fileName()).arg(row).arg(col));
        }
    });
}

void MainWindow::startGrid()
{
    m_sourceDir = m_configPanel->sourceDir();
    m_rows = m_configPanel->rows();
    m_cols = m_configPanel->cols();
    QString filter = m_configPanel->filter();

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
    log(logMsg);

    clearGrid();
    buildGrid(m_rows, m_cols);
    m_currentFilter = filter;
    m_cellPlaylists.clear();

    // Clear and populate playlist widget
    m_sidePanel->playlist()->clear();

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
                // Store playlist for auto-restart and UI
                m_cellPlaylists[{r, c}] = shuffled;
                m_sidePanel->playlist()->setCellPlaylist(r, c, shuffled);
            }
        }
    }

    m_toolBar->setRunning(true);
    m_configPanel->setEnabled(false);
    log(QString("Started %1x%2 grid").arg(m_cols).arg(m_rows));

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
    m_toolBar->setRunning(false);
    m_configPanel->setEnabled(true);
    m_sidePanel->monitor()->clear();
    m_sidePanel->playlist()->clear();
    log("Stopped");
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

            // Connect to monitor widget
            connect(cell, &GridCell::fileChanged, m_sidePanel->monitor(), &MonitorWidget::updateCellStatus);

            // Update playlist highlight when file changes
            connect(cell, &GridCell::fileChanged, this, [this](int row, int col, const QString &path, double, double, bool) {
                m_sidePanel->playlist()->updateCurrentFile(row, col, path);
            });
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
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Intercept key presses from any child widget
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        // Handle keys used in our one-handed layout
        // Number row: 1-6, backtick
        // Letter keys: Q, W, E, R, T, Y, A, S, D, F, G, Z, X, C, V, B
        // Special: Tab, Space, Escape
        if (key == Qt::Key_Space || key == Qt::Key_Tab || key == Qt::Key_Escape ||
            key == Qt::Key_QuoteLeft ||  // backtick
            (key >= Qt::Key_1 && key <= Qt::Key_6) ||
            key == Qt::Key_Q || key == Qt::Key_W || key == Qt::Key_E ||
            key == Qt::Key_R || key == Qt::Key_T || key == Qt::Key_Y ||
            key == Qt::Key_A || key == Qt::Key_S || key == Qt::Key_D ||
            key == Qt::Key_F || key == Qt::Key_G || key == Qt::Key_Z ||
            key == Qt::Key_X || key == Qt::Key_C || key == Qt::Key_V ||
            key == Qt::Key_B) {

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
    case KeyMap::Action::ToggleMute:
        muteAll();
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
    case KeyMap::Action::PanicReset:
        panicReset();
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
    case KeyMap::Action::TogglePauseSelected:
        togglePauseSelected();
        break;
    case KeyMap::Action::ShowPlaylistPicker:
        showPlaylistPicker();
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
        m_toolBar->hide();
        m_configPanel->hide();
        m_sidePanel->hide();
        statusBar()->hide();
        showFullScreen();
        log("Fullscreen ON");
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
        m_toolBar->show();
        m_configPanel->show();
        m_sidePanel->show();
        statusBar()->show();
        log("Fullscreen OFF");
    }
}

void MainWindow::panicReset()
{
    // Exit any fullscreen mode first
    exitFullscreen();

    // Stop all playback and clear grid
    stopGrid();

    log("PANIC! Session reset");
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

    log(QString("Tile fullscreen: [%1,%2]").arg(row).arg(col));
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
    log("Tile fullscreen OFF");
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
        m_statusLabel->setText(QString("Selected: [%1,%2] %3").arg(row).arg(col).arg(cell->currentFile()));
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
    log(QString("Volume: %1%").arg(m_currentVolume));
}

void MainWindow::volumeDownAll()
{
    using namespace MainWindowConstants;
    m_currentVolume = qMax(0, m_currentVolume - kVolumeStep);
    setVolumeAll(m_currentVolume);
    log(QString("Volume: %1%").arg(m_currentVolume));
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

void MainWindow::togglePauseSelected()
{
    if (GridCell *cell = selectedCell()) {
        cell->togglePause();
    }
}

void MainWindow::showPlaylistPicker()
{
    if (!m_selectedCell || m_selectedRow < 0 || m_selectedCol < 0) return;

    // Get playlist for selected cell
    QStringList playlist = m_cellPlaylists.value({m_selectedRow, m_selectedCol});
    if (playlist.isEmpty()) return;

    // Show picker dialog
    PlaylistPicker picker(playlist, this);
    if (picker.exec() == QDialog::Accepted) {
        int index = picker.selectedIndex();
        if (index >= 0) {
            // Jump to index in existing playlist (preserves playlist order)
            m_selectedCell->playIndex(index);
            log(QString("Playing %1").arg(QFileInfo(picker.selectedFile()).fileName()));
        }
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
        log(QString("No media found for [%1,%2]").arg(row).arg(col));
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
        log(QString("[%1,%2]: 1 file, loop=inf").arg(row).arg(col));
    } else {
        log(QString("[%1,%2]: %3 files").arg(row).arg(col).arg(files.size()));
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
                    log(QString("Restarting cell [%1,%2]").arg(r).arg(c));
                    std::shuffle(playlist.begin(), playlist.end(), s_rng);
                    cell->setPlaylist(playlist);
                    cell->play();
                    cell->setVolume(m_currentVolume);
                }
            }
        }
    }
}

void MainWindow::log(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_statusLabel->setText(QString("[%1] %2").arg(timestamp, message));
}
