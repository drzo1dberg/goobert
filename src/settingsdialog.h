#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QKeySequenceEdit>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    void settingsChanged();

private slots:
    void onApply();
    void onReset();
    void onExportStats();
    void onExportSessions();
    void onClearStats();
    void onResetKeyBindings();
    void captureKeyBinding(int row);
    void refreshStats();

private:
    void setupUi();
    QWidget* createGeneralTab();
    QWidget* createPlaybackTab();
    QWidget* createKeyboardTab();
    QWidget* createStatsTab();

    void loadSettings();
    void saveSettings();
    void populateKeyBindings();
    void updateStatsDisplay();
    QString formatDuration(qint64 ms) const;

    QTabWidget *m_tabWidget = nullptr;

    // General tab
    QSpinBox *m_defaultRowsSpin = nullptr;
    QSpinBox *m_defaultColsSpin = nullptr;
    QSpinBox *m_maxGridSizeSpin = nullptr;
    QSpinBox *m_gridSpacingSpin = nullptr;
    QLineEdit *m_defaultMediaPathEdit = nullptr;
    QLineEdit *m_screenshotPathEdit = nullptr;

    // Playback tab
    QDoubleSpinBox *m_seekStepSpin = nullptr;
    QDoubleSpinBox *m_seekStepLongSpin = nullptr;
    QSpinBox *m_volumeStepSpin = nullptr;
    QSpinBox *m_defaultVolumeSpin = nullptr;
    QSpinBox *m_loopCountSpin = nullptr;
    QDoubleSpinBox *m_imageDisplayDurationSpin = nullptr;
    QDoubleSpinBox *m_zoomStepSpin = nullptr;
    QSpinBox *m_rotationStepSpin = nullptr;
    QSpinBox *m_osdDurationSpin = nullptr;
    QSpinBox *m_watchdogIntervalSpin = nullptr;
    QCheckBox *m_skipperEnabledCheck = nullptr;
    QDoubleSpinBox *m_skipPercentSpin = nullptr;

    // Keyboard tab
    QTableWidget *m_keyBindingsTable = nullptr;
    QPushButton *m_resetKeyBindingsBtn = nullptr;
    int m_editingRow = -1;

    // Stats tab
    QCheckBox *m_statsEnabledCheck = nullptr;
    QCheckBox *m_resumePlaybackCheck = nullptr;
    QLabel *m_totalWatchTimeLabel = nullptr;
    QLabel *m_totalFilesLabel = nullptr;
    QLabel *m_totalSessionsLabel = nullptr;
    QLabel *m_avgSessionLabel = nullptr;
    QLabel *m_peakHourLabel = nullptr;
    QLabel *m_peakDayLabel = nullptr;
    QLabel *m_totalSkipsLabel = nullptr;
    QLabel *m_totalScreenshotsLabel = nullptr;
    QLabel *m_todayWatchTimeLabel = nullptr;
    QLabel *m_weekWatchTimeLabel = nullptr;
    QLabel *m_monthWatchTimeLabel = nullptr;
    QTableWidget *m_topFilesTable = nullptr;
    QTableWidget *m_hourlyTable = nullptr;
    QPushButton *m_exportStatsBtn = nullptr;
    QPushButton *m_exportSessionsBtn = nullptr;
    QPushButton *m_clearStatsBtn = nullptr;
};
