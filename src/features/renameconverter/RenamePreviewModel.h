#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

struct RenamePreviewItem
{
    QString currentPath;
    QString newPath;
    QString currentName;
    QString newName;
    bool directory = false;
    QString status;
};

class RenamePreviewModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        CurrentPathRole = Qt::UserRole + 1,
        NewPathRole,
        CurrentNameRole,
        NewNameRole,
        DirectoryRole,
        StatusRole,
        ActualPathRole
    };

    explicit RenamePreviewModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QList<RenamePreviewItem> &items);
    QList<RenamePreviewItem> items() const;
    RenamePreviewItem itemAt(int row) const;
    void updateStatus(int row, const QString &status);
    void replacePathPrefix(const QString &fromPrefix, const QString &toPrefix);
    void clear();

private:
    QString actualPath(const RenamePreviewItem &item) const;

    QList<RenamePreviewItem> m_items;
};
