#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>

class PlaylistPicker : public QDialog
{
    Q_OBJECT

public:
    explicit PlaylistPicker(const QStringList &playlist, QWidget *parent = nullptr);

    [[nodiscard]] QString selectedFile() const { return m_selectedFile; }
    [[nodiscard]] int selectedIndex() const { return m_selectedIndex; }

private slots:
    void onSearchTextChanged(const QString &text);
    void onItemDoubleClicked(QListWidgetItem *item);
    void onItemActivated(QListWidgetItem *item);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateList();
    void selectItem(QListWidgetItem *item);

    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_listWidget = nullptr;
    QLabel *m_countLabel = nullptr;

    QStringList m_fullPlaylist;  // Full paths
    QStringList m_displayNames;  // Just filenames for display
    QString m_searchText;
    QString m_selectedFile;
    int m_selectedIndex = -1;
};
