#include "NamePreviewModel.h"
#include "Config.h"

NamePreviewModel::NamePreviewModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int NamePreviewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.count();
}

QVariant NamePreviewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.count()) {
        return {};
    }

    const auto &item = m_items.at(index.row());
    switch (role) {
    case CurrentPathRole:
        return item.currentPath;
    case NewPathRole:
        return item.newPath;
    case CurrentNameRole:
        return item.currentName;
    case NewNameRole:
        return item.newName;
    case DirectoryRole:
        return item.directory;
    case StatusRole:
        return item.status;
    case ActualPathRole:
        return actualPath(item);
    default:
        return {};
    }
}

QHash<int, QByteArray> NamePreviewModel::roleNames() const
{
    return {
        {CurrentPathRole, "currentPath"},
        {NewPathRole, "newPath"},
        {CurrentNameRole, "currentName"},
        {NewNameRole, "newName"},
        {DirectoryRole, "directory"},
        {StatusRole, "status"},
        {ActualPathRole, "actualPath"}
    };
}

void NamePreviewModel::setItems(const QList<NamePreviewItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

QList<NamePreviewItem> NamePreviewModel::items() const
{
    return m_items;
}

NamePreviewItem NamePreviewModel::itemAt(int row) const
{
    if (row < 0 || row >= m_items.count()) {
        return {};
    }

    return m_items.at(row);
}

void NamePreviewModel::updateStatus(int row, const QString &status)
{
    if (row < 0 || row >= m_items.count()) {
        return;
    }

    m_items[row].status = status;
    const QModelIndex itemIndex = index(row);
    emit dataChanged(itemIndex, itemIndex, {StatusRole, ActualPathRole});
}

void NamePreviewModel::replacePathPrefix(const QString &fromPrefix, const QString &toPrefix)
{
    if (m_items.isEmpty()) {
        return;
    }

    bool changed = false;
    for (auto &item : m_items) {
        changed = ::replacePathPrefix(item.currentPath, fromPrefix, toPrefix) || changed;
        changed = ::replacePathPrefix(item.newPath, fromPrefix, toPrefix) || changed;
    }

    if (!changed) {
        return;
    }

    emit dataChanged(index(0), index(m_items.count() - 1), {CurrentPathRole, NewPathRole, ActualPathRole});
}

void NamePreviewModel::clear()
{
    setItems({});
}

QString NamePreviewModel::actualPath(const NamePreviewItem &item) const
{
    if (item.status == QStringLiteral("已转换")) {
        return item.newPath;
    }

    return item.currentPath;
}