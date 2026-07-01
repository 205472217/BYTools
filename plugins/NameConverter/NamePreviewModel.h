#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

struct NamePreviewItem
{
    QString currentPath;
    QString newPath;
    QString currentName;
    QString newName;
    bool directory = false;
    QString status;
};

class NamePreviewModel : public QAbstractListModel
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

    explicit NamePreviewModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QList<NamePreviewItem> &items);
    QList<NamePreviewItem> items() const;
    NamePreviewItem itemAt(int row) const;
    void updateStatus(int row, const QString &status);
    void replacePathPrefix(const QString &fromPrefix, const QString &toPrefix);
    void clear();

private:
    QString actualPath(const NamePreviewItem &item) const;

    QList<NamePreviewItem> m_items;
};