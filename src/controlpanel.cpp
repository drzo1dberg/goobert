#include "controlpanel.h"
#include "config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QDateTime>
#include <QFileInfo>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QClipboard>
#include <QApplication>
#include <utility>

ControlPanel::ControlPanel(QString sourceDir, QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    Config &cfg = Config::instance();

    // Use provided source dir or config default
    QString initialDir = sourceDir.isEmpty() ? cfg.defaultMediaPath() : std::move(sourceDir);
    m_sourceEdit->setText(initialDir);
}

void ControlPanel::setupUi()
{
    setStyleSheet(R"(
        QWidget { background-color: #1a1a1a; color: #ccc; }
        QPushButton { background-color: #2a2a2a; border: none; padding: 6px 12px; }
        QPushButton:hover { background-color: #3a3a3a; }
        QPushButton:disabled { color: #666; }
        QLineEdit, QSpinBox { background-color: #2a2a2a; border: 1px solid #333; padding: 4px; }
        QSlider::groove:horizontal { background: #333; height: 6px; }
        QSlider::handle:horizontal { background: #666; width: 14px; margin: -4px 0; }
        QTableWidget { background-color: #1e1e1e; gridline-color: #333; }
        QHeaderView::section { background-color: #2a2a2a; padding: 4px; border: none; }
        QToolTip { background-color: #2a2a2a; color: #ddd; border: 1px solid #444; padding: 6px; }
    )");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(4);

    // Row 1: Config + Main Controls
    auto *row1 = new QHBoxLayout();
    row1->setSpacing(8);

    Config &cfg = Config::instance();

    row1->addWidget(new QLabel("Grid"));
    m_colsSpin = new QSpinBox();
    m_colsSpin->setRange(1, 10);
    m_colsSpin->setValue(cfg.defaultCols());
    row1->addWidget(m_colsSpin);

    row1->addWidget(new QLabel("×"));

    m_rowsSpin = new QSpinBox();
    m_rowsSpin->setRange(1, 10);
    m_rowsSpin->setValue(cfg.defaultRows());
    row1->addWidget(m_rowsSpin);

    row1->addSpacing(12);

    row1->addWidget(new QLabel("Source"));
    m_sourceEdit = new QLineEdit();
    m_sourceEdit->setMinimumWidth(300);
    row1->addWidget(m_sourceEdit);

    auto *browseBtn = new QPushButton("...");
    browseBtn->setFocusPolicy(Qt::NoFocus);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Media Directory", m_sourceEdit->text());
        if (!dir.isEmpty()) {
            m_sourceEdit->setText(dir);
        }
    });
    row1->addWidget(browseBtn);

    row1->addSpacing(16);

    m_startBtn = new QPushButton("▶ Start");
    m_startBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_startBtn, &QPushButton::clicked, this, &ControlPanel::startClicked);
    row1->addWidget(m_startBtn);

    m_stopBtn = new QPushButton("■ Stop");
    m_stopBtn->setFocusPolicy(Qt::NoFocus);
    m_stopBtn->setEnabled(false);
    connect(m_stopBtn, &QPushButton::clicked, this, &ControlPanel::stopClicked);
    row1->addWidget(m_stopBtn);

    m_fullscreenBtn = new QPushButton("⛶ Fullscreen");
    m_fullscreenBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &ControlPanel::fullscreenClicked);
    row1->addWidget(m_fullscreenBtn);

    row1->addStretch();

    // Playback controls
    auto *prevBtn = new QPushButton("⏮");
    prevBtn->setFocusPolicy(Qt::NoFocus);
    connect(prevBtn, &QPushButton::clicked, this, &ControlPanel::prevClicked);
    row1->addWidget(prevBtn);

    auto *playPauseBtn = new QPushButton("⏯");
    playPauseBtn->setFocusPolicy(Qt::NoFocus);
    connect(playPauseBtn, &QPushButton::clicked, this, &ControlPanel::playPauseClicked);
    row1->addWidget(playPauseBtn);

    auto *nextBtn = new QPushButton("⏭");
    nextBtn->setFocusPolicy(Qt::NoFocus);
    connect(nextBtn, &QPushButton::clicked, this, &ControlPanel::nextClicked);
    row1->addWidget(nextBtn);

    auto *shuffleBtn = new QPushButton("🔀");
    shuffleBtn->setFocusPolicy(Qt::NoFocus);
    connect(shuffleBtn, &QPushButton::clicked, this, &ControlPanel::shuffleClicked);
    row1->addWidget(shuffleBtn);

    auto *muteBtn = new QPushButton("🔇");
    muteBtn->setFocusPolicy(Qt::NoFocus);
    connect(muteBtn, &QPushButton::clicked, this, &ControlPanel::muteClicked);
    row1->addWidget(muteBtn);

    row1->addWidget(new QLabel("Vol"));
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setFixedWidth(80);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &ControlPanel::volumeChanged);
    m_volumeSlider->setValue(cfg.defaultVolume());  // Set after connect so signal is emitted
    row1->addWidget(m_volumeSlider);

    mainLayout->addLayout(row1);

    // Row 2: Monitor table
    m_monitor = new QTableWidget();
    m_monitor->setColumnCount(3);
    m_monitor->setHorizontalHeaderLabels({"Cell", "Status", "File"});
    m_monitor->horizontalHeader()->setStretchLastSection(true);
    m_monitor->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_monitor->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_monitor->setColumnWidth(1, 200);
    m_monitor->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_monitor->setEditTriggers(QAbstractItemView::NoEditTriggers);  // Disable editing
    m_monitor->setMinimumHeight(80);  // Minimum height, but can grow
    m_monitor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_monitor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_monitor, &QWidget::customContextMenuRequested, this, &ControlPanel::onTableContextMenu);
    mainLayout->addWidget(m_monitor, 1);  // stretch factor 1 so it grows

    // Row 3: Status bar
    auto *row3 = new QHBoxLayout();
    m_pathLabel = new QLabel();
    m_pathLabel->setStyleSheet("color: #666;");
    row3->addWidget(m_pathLabel, 1);

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: #555;");
    row3->addWidget(m_statusLabel);

    mainLayout->addLayout(row3);

    // Connect grid size changes
    connect(m_colsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int cols) {
        emit gridSizeChanged(m_rowsSpin->value(), cols);
    });
    connect(m_rowsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int rows) {
        emit gridSizeChanged(rows, m_colsSpin->value());
    });
}

QString ControlPanel::sourceDir() const
{
    return m_sourceEdit->text();
}

int ControlPanel::rows() const
{
    return m_rowsSpin->value();
}

int ControlPanel::cols() const
{
    return m_colsSpin->value();
}

void ControlPanel::setRunning(bool running)
{
    m_startBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);
    m_sourceEdit->setEnabled(!running);
    m_colsSpin->setEnabled(!running);
    m_rowsSpin->setEnabled(!running);
}

void ControlPanel::setSelectedPath(const QString &path)
{
    m_pathLabel->setText(path);
}

void ControlPanel::log(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_statusLabel->setText(QString("[%1] %2").arg(timestamp, message));
}

void ControlPanel::updateCellStatus(int row, int col, const QString &path, double pos, double dur, bool paused)
{
    QString cellId = QString("%1,%2").arg(row).arg(col);

    // Find or create row
    int tableRow = -1;
    for (int i = 0; i < m_monitor->rowCount(); ++i) {
        if (m_monitor->item(i, 0) && m_monitor->item(i, 0)->text() == cellId) {
            tableRow = i;
            break;
        }
    }

    if (tableRow < 0) {
        tableRow = m_monitor->rowCount();
        m_monitor->insertRow(tableRow);
        m_monitor->setItem(tableRow, 0, new QTableWidgetItem(cellId));
        m_monitor->setItem(tableRow, 1, new QTableWidgetItem());
        m_monitor->setItem(tableRow, 2, new QTableWidgetItem());
    }

    QString status = QString("%1 %2/%3")
        .arg(paused ? "PAUSE" : "PLAY ")
        .arg(formatTime(pos))
        .arg(formatTime(dur));

    QFileInfo fi(path);
    m_monitor->item(tableRow, 1)->setText(status);

    QTableWidgetItem *fileItem = m_monitor->item(tableRow, 2);
    fileItem->setText(fi.fileName());
    fileItem->setData(Qt::UserRole, path);  // Store full path for renaming
}

QString ControlPanel::formatTime(double seconds) const
{
    if (seconds < 0) return "--:--";
    int s = static_cast<int>(seconds);
    int h = s / 3600;
    int m = (s % 3600) / 60;
    int sec = s % 60;
    if (h > 0) {
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
    }
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
}

void ControlPanel::onTableContextMenu(const QPoint &pos)
{
    QTableWidgetItem *item = m_monitor->itemAt(pos);
    if (!item) return;

    int row = item->row();

    // Get cell ID from first column
    QTableWidgetItem *cellItem = m_monitor->item(row, 0);
    if (!cellItem) return;

    QString cellId = cellItem->text();
    QStringList parts = cellId.split(",");
    if (parts.size() != 2) return;

    int cellRow = parts[0].toInt();
    int cellCol = parts[1].toInt();

    // Get file path from third column
    QTableWidgetItem *fileItem = m_monitor->item(row, 2);
    if (!fileItem) return;

    QString fileName = fileItem->text();

    // Need to get full path - we'll need to store it or reconstruct it
    // For now, we'll use the data role to store the full path
    QString fullPath = fileItem->data(Qt::UserRole).toString();
    if (fullPath.isEmpty()) return;

    QMenu menu(this);
    QAction *copyPathAction = menu.addAction("Copy path");
    QAction *renameAction = menu.addAction("Rename file...");
    menu.addSeparator();
    QAction *customSourceAction = menu.addAction("Set custom source...");

    QAction *selected = menu.exec(m_monitor->viewport()->mapToGlobal(pos));
    if (selected == copyPathAction) {
        QApplication::clipboard()->setText(fullPath);
        log("Copied path to clipboard");
    } else if (selected == renameAction) {
        renameFile(cellRow, cellCol, fullPath);
    } else if (selected == customSourceAction) {
        setCustomSource(cellRow, cellCol);
    }
}

void ControlPanel::clearMonitor()
{
    m_monitor->setRowCount(0);
}

void ControlPanel::setCustomSource(int row, int col)
{
    // Let user choose directory or files
    QString path = QFileDialog::getExistingDirectory(this,
        QString("Select source for cell [%1,%2]").arg(row).arg(col),
        m_sourceEdit->text());

    if (path.isEmpty()) return;

    // Scan for media files (we'll emit signal and let MainWindow handle scanning)
    emit customSourceRequested(row, col, QStringList{path});
    log(QString("Custom source for [%1,%2]: %3").arg(row).arg(col).arg(path));
}

void ControlPanel::renameFile(int row, int col, const QString &currentPath)
{
    QFileInfo fileInfo(currentPath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "Error", "File does not exist: " + currentPath);
        return;
    }

    QString oldName = fileInfo.fileName();
    QString extension = fileInfo.suffix();
    QString baseName = fileInfo.completeBaseName();

    bool ok;
    QString newBaseName = QInputDialog::getText(this, "Rename File",
                                                 "New name (without extension):",
                                                 QLineEdit::Normal,
                                                 baseName, &ok);

    if (!ok || newBaseName.isEmpty() || newBaseName == baseName) {
        return;
    }

    QString newFileName = extension.isEmpty() ? newBaseName : newBaseName + "." + extension;
    QString newPath = fileInfo.absolutePath() + "/" + newFileName;

    if (QFile::exists(newPath)) {
        QMessageBox::warning(this, "Error", "A file with that name already exists.");
        return;
    }

    if (!QFile::rename(currentPath, newPath)) {
        QMessageBox::warning(this, "Error", "Failed to rename file.");
        return;
    }

    log(QString("Renamed: %1 → %2").arg(oldName, newFileName));
    emit fileRenamed(currentPath, newPath);
}
