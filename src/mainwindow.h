#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QSplitter>
#include <QVector>
#include <QMap>
#include <QTimer>
#include <QLabel>
#include <memory>
#include <random>
#include "gridcell.h"
#include "toolbar.h"
#include "sidepanel.h"
#include "configpanel.h"

// Constants
namespace MainWindowConstants {
    inline constexpr int kDefaultWidth = 1500;
    inline constexpr int kDefaultHeight = 900;
    inline constexpr int kWallStretchFactor = 9;
    inline constexpr int kControlStretchFactor = 1;
    inline constexpr int kInitialWallSize = 900;
    inline constexpr int kInitialControlSize = 100;
    inline constexpr int kGridMargin = 2;
    inline constexpr int kGridSpacing = 2;
    inline constexpr int kMaxGridSize = 10;
    inline constexpr int kWatchdogIntervalMs = 5000;
    inline constexpr int kShuffleNextDelayMs = 200;
    inline constexpr int kVolumeStep = 5;
    inline constexpr double kSeekStepSeconds = 5.0;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString sourceDir, QWidget *parent = nullptr);
    ~MainWindow() override;

    // Non-copyable
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void startGrid();
    void stopGrid();
    void toggleFullscreen();
    void exitFullscreen();

    void playPauseAll();
    void nextAll();
    void prevAll();
    void shuffleAll();
    void shuffleThenNextAll();
    void muteAll();
    void setVolumeAll(int volume);
    void volumeUpAll();
    void volumeDownAll();
    void toggleTileFullscreen();

    // Selected cell actions
    void toggleLoopSelected();
    void frameStepSelected();
    void frameBackStepSelected();
    void rotateSelected();
    void zoomInSelected();
    void zoomOutSelected();
    void seekSelected(double seconds);
    void screenshotSelected();

    void onCellSelected(int row, int col);
    void onCellDoubleClicked(int row, int col);
    void onFileRenamed(const QString &oldPath, const QString &newPath);
    void onCustomSource(int row, int col, const QStringList &paths);
    void navigateSelection(int colDelta, int rowDelta);
    void watchdogCheck();
    void log(const QString &message);

private:
    void setupUi();
    void buildGrid(int rows, int cols);
    void clearGrid();
    void enterTileFullscreen(int row, int col);
    void exitTileFullscreen();
    [[nodiscard]] GridCell* selectedCell() const noexcept;

    QString m_sourceDir;
    int m_rows = 3;
    int m_cols = 3;

    QWidget *m_centralWidget = nullptr;
    QSplitter *m_splitter = nullptr;
    QWidget *m_wallContainer = nullptr;
    QGridLayout *m_gridLayout = nullptr;

    // New UI components
    ToolBar *m_toolBar = nullptr;
    SidePanel *m_sidePanel = nullptr;
    ConfigPanel *m_configPanel = nullptr;
    QLabel *m_statusLabel = nullptr;

    QVector<GridCell*> m_cells;
    QMap<QPair<int,int>, GridCell*> m_cellMap;

    bool m_isFullscreen = false;
    bool m_isTileFullscreen = false;
    GridCell *m_fullscreenCell = nullptr;
    GridCell *m_selectedCell = nullptr;
    int m_selectedRow = -1;
    int m_selectedCol = -1;
    int m_currentVolume = 30;

    // Watchdog for auto-restart
    QTimer *m_watchdogTimer = nullptr;
    QMap<QPair<int,int>, QStringList> m_cellPlaylists;
    QString m_currentFilter;

    // Random number generator for shuffle operations
    static inline std::mt19937 s_rng{std::random_device{}()};
};
