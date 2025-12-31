#include "monitorwidget.h"
#include "theme.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>
#include <QClipboard>
#include <QApplication>

MonitorWidget::MonitorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void MonitorWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget();
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"Cell", "Status", "File"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 140);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);

    // Apply 2026 theme
    m_table->setStyleSheet(Theme::tableStyle());

    connect(m_table, &QWidget::customContextMenuRequested, this, &MonitorWidget::onContextMenu);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &MonitorWidget::onItemDoubleClicked);

    layout->addWidget(m_table);
}

void MonitorWidget::updateCellStatus(int row, int col, const QString &path, double pos, double dur, bool paused)
{
    QString cellId = QString("%1,%2").arg(row).arg(col);

    // Find or create row
    int tableRow = -1;
    for (int i = 0; i < m_table->rowCount(); ++i) {
        if (m_table->item(i, 0) && m_table->item(i, 0)->text() == cellId) {
            tableRow = i;
            break;
        }
    }

    if (tableRow < 0) {
        tableRow = m_table->rowCount();
        m_table->insertRow(tableRow);
        m_table->setItem(tableRow, 0, new QTableWidgetItem(cellId));
        m_table->setItem(tableRow, 1, new QTableWidgetItem());
        m_table->setItem(tableRow, 2, new QTableWidgetItem());
    }

    // Status with play/pause indicator
    QString statusIcon = paused ? "||" : ">";
    QString status = QString("%1 %2 / %3")
        .arg(statusIcon)
        .arg(formatTime(pos))
        .arg(formatTime(dur));

    QFileInfo fi(path);
    m_table->item(tableRow, 1)->setText(status);

    QTableWidgetItem *fileItem = m_table->item(tableRow, 2);
    fileItem->setText(fi.fileName());
    fileItem->setData(Qt::UserRole, path);  // Store full path
    fileItem->setToolTip(path);
}

void MonitorWidget::clear()
{
    m_table->setRowCount(0);
}

QString MonitorWidget::formatTime(double seconds) const
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

void MonitorWidget::onContextMenu(const QPoint &pos)
{
    QTableWidgetItem *item = m_table->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QTableWidgetItem *cellItem = m_table->item(row, 0);
    QTableWidgetItem *fileItem = m_table->item(row, 2);
    if (!cellItem || !fileItem) return;

    QString cellId = cellItem->text();
    QStringList parts = cellId.split(",");
    if (parts.size() != 2) return;

    int cellRow = parts[0].toInt();
    int cellCol = parts[1].toInt();
    QString fullPath = fileItem->data(Qt::UserRole).toString();

    QMenu menu(this);
    QAction *copyPathAction = menu.addAction("Copy path");
    QAction *renameAction = menu.addAction("Rename file...");
    menu.addSeparator();
    QAction *customSourceAction = menu.addAction("Set custom source...");

    QAction *selected = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (selected == copyPathAction && !fullPath.isEmpty()) {
        QApplication::clipboard()->setText(fullPath);
    } else if (selected == renameAction && !fullPath.isEmpty()) {
        renameFile(cellRow, cellCol, fullPath);
    } else if (selected == customSourceAction) {
        setCustomSource(cellRow, cellCol);
    }
}

void MonitorWidget::onItemDoubleClicked(QTableWidgetItem *item)
{
    if (!item) return;

    int row = item->row();
    QTableWidgetItem *cellItem = m_table->item(row, 0);
    if (!cellItem) return;

    QString cellId = cellItem->text();
    QStringList parts = cellId.split(",");
    if (parts.size() == 2) {
        emit cellSelected(parts[0].toInt(), parts[1].toInt());
    }
}

void MonitorWidget::renameFile(int row, int col, const QString &currentPath)
{
    Q_UNUSED(row);
    Q_UNUSED(col);

    QFileInfo fileInfo(currentPath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "Error", "File does not exist: " + currentPath);
        return;
    }

    QString baseName = fileInfo.completeBaseName();
    QString extension = fileInfo.suffix();

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

    emit fileRenamed(currentPath, newPath);
}

void MonitorWidget::setCustomSource(int row, int col)
{
    bool ok;
    QString path = QInputDialog::getText(this,
        QString("Set source for cell [%1,%2]").arg(row).arg(col),
        "Path:",
        QLineEdit::Normal,
        QString(),
        &ok);

    if (!ok || path.trimmed().isEmpty()) return;

    emit customSourceRequested(row, col, QStringList{path.trimmed()});
}
