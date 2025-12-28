#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QSplitter>
#include <QVector>
#include <QMap>
#include <QTimer>
#include <memory>
#include <random>
#include "gridcell.h"
#include "controlpanel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString sourceDir, QWidget *parent = nullptr);
    ~MainWindow();

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
    void nextAllIfNotLooping();  // Grid-sync aware
    void prevAll();
    void shuffleAll();
    void shuffleThenNextAll();   // Chained: shuffle, delay, next
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
    void navigateSelection(int colDelta, int rowDelta);

private:
    void setupUi();
    void buildGrid(int rows, int cols);
    void clearGrid();
    void enterTileFullscreen(int row, int col);
    void exitTileFullscreen();
    GridCell* selectedCell() const;

    QString m_sourceDir;
    int m_rows = 3;
    int m_cols = 3;

    QWidget *m_centralWidget = nullptr;
    QSplitter *m_splitter = nullptr;
    QWidget *m_wallContainer = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    ControlPanel *m_controlPanel = nullptr;

    QVector<GridCell*> m_cells;
    QMap<QPair<int,int>, GridCell*> m_cellMap;

    bool m_isFullscreen = false;
    bool m_isTileFullscreen = false;
    GridCell *m_fullscreenCell = nullptr;
    GridCell *m_selectedCell = nullptr;
    int m_selectedRow = -1;
    int m_selectedCol = -1;
    int m_currentVolume = 30;  // Track current volume level

    // Random number generator for shuffle operations
    static inline std::mt19937 s_rng{std::random_device{}()};
};
