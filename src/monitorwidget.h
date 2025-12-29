#pragma once

#include <QWidget>
#include <QTableWidget>

class MonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorWidget(QWidget *parent = nullptr);

    void updateCellStatus(int row, int col, const QString &path, double pos, double dur, bool paused);
    void clear();

signals:
    void cellSelected(int row, int col);
    void fileRenamed(const QString &oldPath, const QString &newPath);
    void customSourceRequested(int row, int col, const QStringList &paths);

private slots:
    void onContextMenu(const QPoint &pos);
    void onItemDoubleClicked(QTableWidgetItem *item);

private:
    void setupUi();
    [[nodiscard]] QString formatTime(double seconds) const;
    void renameFile(int row, int col, const QString &currentPath);
    void setCustomSource(int row, int col);

    QTableWidget *m_table = nullptr;
};
