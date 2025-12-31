#include "settingsdialog.h"
#include "config.h"
#include "keymap.h"
#include "statsmanager.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QKeyEvent>
#include <QDateTime>
#include <QMenu>
#include <QClipboard>
#include <QApplication>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumSize(700, 600);
    setupUi();
    loadSettings();
    updateStatsDisplay();
}

void SettingsDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(Theme::Spacing::MD);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(), "General");
    m_tabWidget->addTab(createPlaybackTab(), "Playback");
    m_tabWidget->addTab(createKeyboardTab(), "Keyboard");
    m_tabWidget->addTab(createStatsTab(), "Statistics");

    mainLayout->addWidget(m_tabWidget);

    // Dialog buttons
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    auto *resetBtn = new QPushButton("Reset to Defaults");
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onReset);
    buttonLayout->addWidget(resetBtn);

    auto *applyBtn = new QPushButton("Apply");
    connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    buttonLayout->addWidget(applyBtn);

    auto *closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);

    mainLayout->addLayout(buttonLayout);

    setStyleSheet(Theme::dialogStyle());
}

QWidget* SettingsDialog::createGeneralTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    // Grid settings
    auto *gridGroup = new QGroupBox("Grid Settings");
    auto *gridLayout = new QFormLayout(gridGroup);

    m_defaultRowsSpin = new QSpinBox;
    m_defaultRowsSpin->setRange(1, 10);
    gridLayout->addRow("Default Rows:", m_defaultRowsSpin);

    m_defaultColsSpin = new QSpinBox;
    m_defaultColsSpin->setRange(1, 10);
    gridLayout->addRow("Default Columns:", m_defaultColsSpin);

    m_maxGridSizeSpin = new QSpinBox;
    m_maxGridSizeSpin->setRange(1, 20);
    gridLayout->addRow("Max Grid Size:", m_maxGridSizeSpin);

    m_gridSpacingSpin = new QSpinBox;
    m_gridSpacingSpin->setRange(0, 20);
    gridLayout->addRow("Grid Spacing (px):", m_gridSpacingSpin);

    layout->addWidget(gridGroup);

    // Paths
    auto *pathsGroup = new QGroupBox("Paths");
    auto *pathsLayout = new QFormLayout(pathsGroup);

    auto *mediaPathLayout = new QHBoxLayout;
    m_defaultMediaPathEdit = new QLineEdit;
    mediaPathLayout->addWidget(m_defaultMediaPathEdit);
    auto *browseMediaBtn = new QPushButton("...");
    browseMediaBtn->setFixedWidth(30);
    connect(browseMediaBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Media Directory", m_defaultMediaPathEdit->text());
        if (!dir.isEmpty()) m_defaultMediaPathEdit->setText(dir);
    });
    mediaPathLayout->addWidget(browseMediaBtn);
    pathsLayout->addRow("Default Media Path:", mediaPathLayout);

    auto *screenshotPathLayout = new QHBoxLayout;
    m_screenshotPathEdit = new QLineEdit;
    screenshotPathLayout->addWidget(m_screenshotPathEdit);
    auto *browseScreenshotBtn = new QPushButton("...");
    browseScreenshotBtn->setFixedWidth(30);
    connect(browseScreenshotBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Screenshot Directory", m_screenshotPathEdit->text());
        if (!dir.isEmpty()) m_screenshotPathEdit->setText(dir);
    });
    screenshotPathLayout->addWidget(browseScreenshotBtn);
    pathsLayout->addRow("Screenshot Path:", screenshotPathLayout);

    layout->addWidget(pathsGroup);
    layout->addStretch();

    return widget;
}

QWidget* SettingsDialog::createPlaybackTab()
{
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    // Seeking
    auto *seekGroup = new QGroupBox("Seeking");
    auto *seekLayout = new QFormLayout(seekGroup);

    m_seekStepSpin = new QDoubleSpinBox;
    m_seekStepSpin->setRange(1.0, 60.0);
    m_seekStepSpin->setSuffix(" sec");
    seekLayout->addRow("Short Seek Step:", m_seekStepSpin);

    m_seekStepLongSpin = new QDoubleSpinBox;
    m_seekStepLongSpin->setRange(10.0, 600.0);
    m_seekStepLongSpin->setSuffix(" sec");
    seekLayout->addRow("Long Seek Step:", m_seekStepLongSpin);

    layout->addWidget(seekGroup);

    // Volume
    auto *volumeGroup = new QGroupBox("Volume");
    auto *volumeLayout = new QFormLayout(volumeGroup);

    m_defaultVolumeSpin = new QSpinBox;
    m_defaultVolumeSpin->setRange(0, 100);
    m_defaultVolumeSpin->setSuffix(" %");
    volumeLayout->addRow("Default Volume:", m_defaultVolumeSpin);

    m_volumeStepSpin = new QSpinBox;
    m_volumeStepSpin->setRange(1, 25);
    volumeLayout->addRow("Volume Step:", m_volumeStepSpin);

    layout->addWidget(volumeGroup);

    // Looping
    auto *loopGroup = new QGroupBox("Looping & Display");
    auto *loopLayout = new QFormLayout(loopGroup);

    m_loopCountSpin = new QSpinBox;
    m_loopCountSpin->setRange(1, 99);
    loopLayout->addRow("Default Loop Count:", m_loopCountSpin);

    m_imageDisplayDurationSpin = new QDoubleSpinBox;
    m_imageDisplayDurationSpin->setRange(0.5, 60.0);
    m_imageDisplayDurationSpin->setSuffix(" sec");
    loopLayout->addRow("Image Display Duration:", m_imageDisplayDurationSpin);

    layout->addWidget(loopGroup);

    // Video Controls
    auto *videoGroup = new QGroupBox("Video Controls");
    auto *videoLayout = new QFormLayout(videoGroup);

    m_zoomStepSpin = new QDoubleSpinBox;
    m_zoomStepSpin->setRange(0.05, 0.5);
    m_zoomStepSpin->setSingleStep(0.05);
    videoLayout->addRow("Zoom Step:", m_zoomStepSpin);

    m_rotationStepSpin = new QSpinBox;
    m_rotationStepSpin->setRange(45, 180);
    m_rotationStepSpin->setSingleStep(45);
    m_rotationStepSpin->setSuffix(" deg");
    videoLayout->addRow("Rotation Step:", m_rotationStepSpin);

    m_osdDurationSpin = new QSpinBox;
    m_osdDurationSpin->setRange(500, 5000);
    m_osdDurationSpin->setSingleStep(100);
    m_osdDurationSpin->setSuffix(" ms");
    videoLayout->addRow("OSD Duration:", m_osdDurationSpin);

    m_watchdogIntervalSpin = new QSpinBox;
    m_watchdogIntervalSpin->setRange(1000, 30000);
    m_watchdogIntervalSpin->setSingleStep(1000);
    m_watchdogIntervalSpin->setSuffix(" ms");
    videoLayout->addRow("Watchdog Interval:", m_watchdogIntervalSpin);

    layout->addWidget(videoGroup);

    // Skipper
    auto *skipperGroup = new QGroupBox("Skipper");
    auto *skipperLayout = new QFormLayout(skipperGroup);

    m_skipperEnabledCheck = new QCheckBox("Enable Skipper");
    skipperLayout->addRow(m_skipperEnabledCheck);

    m_skipPercentSpin = new QDoubleSpinBox;
    m_skipPercentSpin->setRange(0.0, 1.0);
    m_skipPercentSpin->setSingleStep(0.05);
    m_skipPercentSpin->setDecimals(2);
    skipperLayout->addRow("Skip Percent:", m_skipPercentSpin);

    layout->addWidget(skipperGroup);
    layout->addStretch();

    scrollArea->setWidget(widget);
    return scrollArea;
}

QWidget* SettingsDialog::createKeyboardTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *label = new QLabel("Click on a key binding to change it. Press the new key combination.");
    layout->addWidget(label);

    m_keyBindingsTable = new QTableWidget;
    m_keyBindingsTable->setColumnCount(3);
    m_keyBindingsTable->setHorizontalHeaderLabels({"Action", "Key", "Description"});
    m_keyBindingsTable->horizontalHeader()->setStretchLastSection(true);
    m_keyBindingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_keyBindingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_keyBindingsTable->verticalHeader()->setVisible(false);

    connect(m_keyBindingsTable, &QTableWidget::cellDoubleClicked, this, &SettingsDialog::captureKeyBinding);

    populateKeyBindings();
    layout->addWidget(m_keyBindingsTable);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_resetKeyBindingsBtn = new QPushButton("Reset to Defaults");
    connect(m_resetKeyBindingsBtn, &QPushButton::clicked, this, &SettingsDialog::onResetKeyBindings);
    buttonLayout->addWidget(m_resetKeyBindingsBtn);

    layout->addLayout(buttonLayout);

    return widget;
}

QWidget* SettingsDialog::createStatsTab()
{
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    // Settings
    auto *settingsGroup = new QGroupBox("Settings");
    auto *settingsLayout = new QVBoxLayout(settingsGroup);

    m_statsEnabledCheck = new QCheckBox("Enable Statistics Tracking");
    settingsLayout->addWidget(m_statsEnabledCheck);

    m_resumePlaybackCheck = new QCheckBox("Resume from Last Position");
    settingsLayout->addWidget(m_resumePlaybackCheck);

    layout->addWidget(settingsGroup);

    // Summary
    auto *summaryGroup = new QGroupBox("Summary");
    auto *summaryLayout = new QFormLayout(summaryGroup);

    m_totalWatchTimeLabel = new QLabel("--");
    summaryLayout->addRow("Total Watch Time:", m_totalWatchTimeLabel);

    m_totalFilesLabel = new QLabel("--");
    summaryLayout->addRow("Files Tracked:", m_totalFilesLabel);

    m_totalSessionsLabel = new QLabel("--");
    summaryLayout->addRow("Total Sessions:", m_totalSessionsLabel);

    m_avgSessionLabel = new QLabel("--");
    summaryLayout->addRow("Avg Session Length:", m_avgSessionLabel);

    m_peakHourLabel = new QLabel("--");
    summaryLayout->addRow("Peak Hour:", m_peakHourLabel);

    m_peakDayLabel = new QLabel("--");
    summaryLayout->addRow("Peak Day:", m_peakDayLabel);

    m_totalSkipsLabel = new QLabel("--");
    summaryLayout->addRow("Total Skips:", m_totalSkipsLabel);

    m_totalScreenshotsLabel = new QLabel("--");
    summaryLayout->addRow("Total Screenshots:", m_totalScreenshotsLabel);

    layout->addWidget(summaryGroup);

    // Time Range Stats
    auto *timeGroup = new QGroupBox("Time Range Stats");
    auto *timeLayout = new QFormLayout(timeGroup);

    m_todayWatchTimeLabel = new QLabel("--");
    timeLayout->addRow("Today:", m_todayWatchTimeLabel);

    m_weekWatchTimeLabel = new QLabel("--");
    timeLayout->addRow("This Week:", m_weekWatchTimeLabel);

    m_monthWatchTimeLabel = new QLabel("--");
    timeLayout->addRow("This Month:", m_monthWatchTimeLabel);

    layout->addWidget(timeGroup);

    // Top Files
    auto *topFilesGroup = new QGroupBox("Most Watched Files");
    auto *topFilesLayout = new QVBoxLayout(topFilesGroup);

    m_topFilesTable = new QTableWidget;
    m_topFilesTable->setColumnCount(8);
    m_topFilesTable->setHorizontalHeaderLabels({"File", "Watch Time", "Plays", "Skips", "Loops", "Avg %", "Last Watched", "Path"});
    m_topFilesTable->horizontalHeader()->setStretchLastSection(true);
    m_topFilesTable->setMaximumHeight(250);
    m_topFilesTable->verticalHeader()->setVisible(false);
    m_topFilesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_topFilesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_topFilesTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        int row = m_topFilesTable->rowAt(pos.y());
        if (row >= 0 && m_topFilesTable->item(row, 7)) {
            QString path = m_topFilesTable->item(row, 7)->text();
            QMenu menu;
            QAction *copyAction = menu.addAction("Copy Path");
            if (menu.exec(m_topFilesTable->viewport()->mapToGlobal(pos)) == copyAction) {
                QApplication::clipboard()->setText(path);
            }
        }
    });
    topFilesLayout->addWidget(m_topFilesTable);

    layout->addWidget(topFilesGroup);

    // Hourly Distribution
    auto *hourlyGroup = new QGroupBox("Hourly Distribution");
    auto *hourlyLayout = new QVBoxLayout(hourlyGroup);

    m_hourlyTable = new QTableWidget;
    m_hourlyTable->setColumnCount(24);
    m_hourlyTable->setRowCount(1);
    QStringList hourHeaders;
    for (int i = 0; i < 24; ++i) hourHeaders << QString::number(i);
    m_hourlyTable->setHorizontalHeaderLabels(hourHeaders);
    m_hourlyTable->setVerticalHeaderLabels({"Sessions"});
    m_hourlyTable->setMaximumHeight(60);
    hourlyLayout->addWidget(m_hourlyTable);

    layout->addWidget(hourlyGroup);

    // Buttons
    auto *buttonLayout = new QHBoxLayout;

    auto *refreshBtn = new QPushButton("Refresh");
    connect(refreshBtn, &QPushButton::clicked, this, &SettingsDialog::refreshStats);
    buttonLayout->addWidget(refreshBtn);

    buttonLayout->addStretch();

    m_exportStatsBtn = new QPushButton("Export File Stats (CSV)");
    connect(m_exportStatsBtn, &QPushButton::clicked, this, &SettingsDialog::onExportStats);
    buttonLayout->addWidget(m_exportStatsBtn);

    m_exportSessionsBtn = new QPushButton("Export Sessions (CSV)");
    connect(m_exportSessionsBtn, &QPushButton::clicked, this, &SettingsDialog::onExportSessions);
    buttonLayout->addWidget(m_exportSessionsBtn);

    m_clearStatsBtn = new QPushButton("Clear All Stats");
    connect(m_clearStatsBtn, &QPushButton::clicked, this, &SettingsDialog::onClearStats);
    buttonLayout->addWidget(m_clearStatsBtn);

    layout->addLayout(buttonLayout);
    layout->addStretch();

    scrollArea->setWidget(widget);
    return scrollArea;
}

void SettingsDialog::loadSettings()
{
    Config &config = Config::instance();

    // General
    m_defaultRowsSpin->setValue(config.defaultRows());
    m_defaultColsSpin->setValue(config.defaultCols());
    m_maxGridSizeSpin->setValue(config.maxGridSize());
    m_gridSpacingSpin->setValue(config.gridSpacing());
    m_defaultMediaPathEdit->setText(config.defaultMediaPath());
    m_screenshotPathEdit->setText(config.screenshotPath());

    // Playback
    m_seekStepSpin->setValue(config.seekStepSeconds());
    m_seekStepLongSpin->setValue(config.seekStepLongSeconds());
    m_defaultVolumeSpin->setValue(config.defaultVolume());
    m_volumeStepSpin->setValue(config.volumeStep());
    m_loopCountSpin->setValue(config.loopCount());
    m_imageDisplayDurationSpin->setValue(config.imageDisplayDuration());
    m_zoomStepSpin->setValue(config.zoomStep());
    m_rotationStepSpin->setValue(config.rotationStep());
    m_osdDurationSpin->setValue(config.osdDurationMs());
    m_watchdogIntervalSpin->setValue(config.watchdogIntervalMs());
    m_skipperEnabledCheck->setChecked(config.skipperEnabled());
    m_skipPercentSpin->setValue(config.skipPercent());

    // Stats
    m_statsEnabledCheck->setChecked(config.statsEnabled());
    m_resumePlaybackCheck->setChecked(config.resumePlaybackEnabled());
}

void SettingsDialog::saveSettings()
{
    Config &config = Config::instance();

    // General
    config.setDefaultRows(m_defaultRowsSpin->value());
    config.setDefaultCols(m_defaultColsSpin->value());
    config.setMaxGridSize(m_maxGridSizeSpin->value());
    config.setGridSpacing(m_gridSpacingSpin->value());
    config.setDefaultMediaPath(m_defaultMediaPathEdit->text());
    config.setScreenshotPath(m_screenshotPathEdit->text());

    // Playback
    config.setSeekStepSeconds(m_seekStepSpin->value());
    config.setSeekStepLongSeconds(m_seekStepLongSpin->value());
    config.setDefaultVolume(m_defaultVolumeSpin->value());
    config.setVolumeStep(m_volumeStepSpin->value());
    config.setLoopCount(m_loopCountSpin->value());
    config.setImageDisplayDuration(m_imageDisplayDurationSpin->value());
    config.setZoomStep(m_zoomStepSpin->value());
    config.setRotationStep(m_rotationStepSpin->value());
    config.setOsdDurationMs(m_osdDurationSpin->value());
    config.setWatchdogIntervalMs(m_watchdogIntervalSpin->value());
    config.setSkipperEnabled(m_skipperEnabledCheck->isChecked());
    config.setSkipPercent(m_skipPercentSpin->value());

    // Stats
    config.setStatsEnabled(m_statsEnabledCheck->isChecked());
    config.setResumePlaybackEnabled(m_resumePlaybackCheck->isChecked());
}

void SettingsDialog::populateKeyBindings()
{
    KeyMap &keymap = KeyMap::instance();
    auto bindings = keymap.getAllBindings();

    m_keyBindingsTable->setRowCount(bindings.size());

    int row = 0;
    for (const auto &pair : bindings) {
        auto action = pair.first;
        auto binding = pair.second;

        auto *actionItem = new QTableWidgetItem(keymap.actionToString(action));
        actionItem->setData(Qt::UserRole, static_cast<int>(action));
        m_keyBindingsTable->setItem(row, 0, actionItem);

        QString keyStr = keymap.getKeyDescription(binding.key, binding.modifiers);
        m_keyBindingsTable->setItem(row, 1, new QTableWidgetItem(keyStr));

        QString desc = keymap.getActionDescription(action);
        m_keyBindingsTable->setItem(row, 2, new QTableWidgetItem(desc));

        ++row;
    }

    m_keyBindingsTable->resizeColumnsToContents();
}

void SettingsDialog::captureKeyBinding(int row)
{
    m_editingRow = row;
    m_keyBindingsTable->item(row, 1)->setText("[Press key...]");

    // Install event filter to capture key
    m_keyBindingsTable->installEventFilter(this);
    m_keyBindingsTable->setFocus();
}

void SettingsDialog::updateStatsDisplay()
{
    StatsManager &stats = StatsManager::instance();

    if (!stats.isInitialized()) {
        m_totalWatchTimeLabel->setText("Statistics not initialized");
        return;
    }

    // Summary
    m_totalWatchTimeLabel->setText(formatDuration(stats.getTotalWatchTime()));
    m_totalFilesLabel->setText(QString::number(stats.getTotalFilesTracked()));

    auto sessions = stats.getRecentSessions(1000);
    m_totalSessionsLabel->setText(QString::number(sessions.size()));

    double avgSession = stats.getAverageSessionLength();
    m_avgSessionLabel->setText(formatDuration(static_cast<qint64>(avgSession)));

    int peakHour = stats.getPeakHour();
    m_peakHourLabel->setText(QString("%1:00 - %2:00").arg(peakHour).arg((peakHour + 1) % 24));

    static const QStringList dayNames = {"", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    int peakDay = stats.getPeakDayOfWeek();
    m_peakDayLabel->setText(peakDay >= 1 && peakDay <= 7 ? dayNames[peakDay] : "--");

    m_totalSkipsLabel->setText(QString::number(stats.getTotalSkips()));
    m_totalScreenshotsLabel->setText(QString::number(stats.getTotalScreenshots()));

    // Time range stats
    auto todayStats = stats.getStatsForToday();
    m_todayWatchTimeLabel->setText(QString("%1 (%2 sessions)").arg(formatDuration(todayStats.totalWatchMs)).arg(todayStats.sessionCount));

    auto weekStats = stats.getStatsForThisWeek();
    m_weekWatchTimeLabel->setText(QString("%1 (%2 sessions)").arg(formatDuration(weekStats.totalWatchMs)).arg(weekStats.sessionCount));

    auto monthStats = stats.getStatsForThisMonth();
    m_monthWatchTimeLabel->setText(QString("%1 (%2 sessions)").arg(formatDuration(monthStats.totalWatchMs)).arg(monthStats.sessionCount));

    // Top files
    auto topFiles = stats.getMostWatched(10);
    m_topFilesTable->setRowCount(topFiles.size());
    for (int i = 0; i < topFiles.size(); ++i) {
        const auto &file = topFiles[i];
        QString filename = file.filePath.section('/', -1);
        m_topFilesTable->setItem(i, 0, new QTableWidgetItem(filename));
        m_topFilesTable->item(i, 0)->setToolTip(file.filePath);
        m_topFilesTable->setItem(i, 1, new QTableWidgetItem(formatDuration(file.totalWatchMs)));
        m_topFilesTable->setItem(i, 2, new QTableWidgetItem(QString::number(file.playCount)));
        m_topFilesTable->setItem(i, 3, new QTableWidgetItem(QString::number(file.skipCount)));
        m_topFilesTable->setItem(i, 4, new QTableWidgetItem(QString::number(file.loopCount)));
        m_topFilesTable->setItem(i, 5, new QTableWidgetItem(QString("%1%").arg(file.avgWatchPercent, 0, 'f', 0)));

        QString lastWatched = file.lastWatchedAt > 0
            ? QDateTime::fromMSecsSinceEpoch(file.lastWatchedAt).toString("yyyy-MM-dd HH:mm")
            : "--";
        m_topFilesTable->setItem(i, 6, new QTableWidgetItem(lastWatched));
        m_topFilesTable->setItem(i, 7, new QTableWidgetItem(file.filePath));
    }
    m_topFilesTable->resizeColumnsToContents();

    // Hourly distribution
    auto hourly = stats.getHourlyDistribution();
    for (int h = 0; h < 24 && h < hourly.size(); ++h) {
        m_hourlyTable->setItem(0, h, new QTableWidgetItem(QString::number(hourly[h].sessionCount)));
    }
}

QString SettingsDialog::formatDuration(qint64 ms) const
{
    if (ms <= 0) return "0s";

    qint64 seconds = ms / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;

    seconds %= 60;
    minutes %= 60;

    if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds);
    } else if (minutes > 0) {
        return QString("%1m %2s").arg(minutes).arg(seconds);
    } else {
        return QString("%1s").arg(seconds);
    }
}

void SettingsDialog::onApply()
{
    saveSettings();
    emit settingsChanged();
}

void SettingsDialog::onReset()
{
    if (QMessageBox::question(this, "Reset Settings",
            "Reset all settings to defaults?",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        Config::instance().resetToDefaults();
        loadSettings();
    }
}

void SettingsDialog::onExportStats()
{
    QString path = QFileDialog::getSaveFileName(this, "Export Statistics", "", "CSV Files (*.csv)");
    if (!path.isEmpty()) {
        if (StatsManager::instance().exportToCsv(path)) {
            QMessageBox::information(this, "Export", "Statistics exported successfully.");
        } else {
            QMessageBox::warning(this, "Export", "Failed to export statistics.");
        }
    }
}

void SettingsDialog::onExportSessions()
{
    QString path = QFileDialog::getSaveFileName(this, "Export Sessions", "", "CSV Files (*.csv)");
    if (!path.isEmpty()) {
        if (StatsManager::instance().exportSessionsToCsv(path)) {
            QMessageBox::information(this, "Export", "Sessions exported successfully.");
        } else {
            QMessageBox::warning(this, "Export", "Failed to export sessions.");
        }
    }
}

void SettingsDialog::onClearStats()
{
    if (QMessageBox::question(this, "Clear Statistics",
            "Are you sure you want to delete all statistics? This cannot be undone.",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        StatsManager::instance().clearAllStats();
        updateStatsDisplay();
    }
}

void SettingsDialog::onResetKeyBindings()
{
    if (QMessageBox::question(this, "Reset Key Bindings",
            "Reset all key bindings to defaults?",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        KeyMap::instance().resetToDefaults();
        populateKeyBindings();
    }
}

void SettingsDialog::refreshStats()
{
    updateStatsDisplay();
}
