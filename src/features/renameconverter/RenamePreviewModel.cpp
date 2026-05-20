#include "RenamePreviewModel.h"

#include <QDir>

namespace {

bool replacePrefix(QString &path, const QString &fromPrefix, const QString &toPrefix)
{
    const QString cleanedPath = QDir::cleanPath(path);
    const QString cleanedFrom = QDir::cleanPath(fromPrefix);
    const QString cleanedTo = QDir::cleanPath(toPrefix);
    const QString childPrefix = cleanedFrom + QLatin1Char('/');

    if (cleanedPath.compare(cleanedFrom, Qt::CaseInsensitive) == 0) {
        path = cleanedTo;
        return true;
    }

    if (cleanedPath.startsWith(childPrefix, Qt::CaseInsensitive)) {
        path = cleanedTo + cleanedPath.mid(cleanedFrom.length());
        return true;
    }

    return false;
}

}

RenamePreviewModel::RenamePreviewModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RenamePreviewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.count();
}

QVariant RenamePreviewModel::data(const QModelIndex &index, int role) const
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

QHash<int, QByteArray> RenamePreviewModel::roleNames() const
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

void RenamePreviewModel::setItems(const QList<RenamePreviewItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

QList<RenamePreviewItem> RenamePreviewModel::items() const
{
    return m_items;
}

RenamePreviewItem RenamePreviewModel::itemAt(int row) const
{
    if (row < 0 || row >= m_items.count()) {
        return {};
    }

    return m_items.at(row);
}

void RenamePreviewModel::updateStatus(int row, const QString &status)
{
    if (row < 0 || row >= m_items.count()) {
        return;
    }

    m_items[row].status = status;
    const QModelIndex itemIndex = index(row);
    emit dataChanged(itemIndex, itemIndex, {StatusRole, ActualPathRole});
}

void RenamePreviewModel::replacePathPrefix(const QString &fromPrefix, const QString &toPrefix)
{
    if (m_items.isEmpty()) {
        return;
    }

    bool changed = false;
    for (auto &item : m_items) {
        changed = replacePrefix(item.currentPath, fromPrefix, toPrefix) || changed;
        changed = replacePrefix(item.newPath, fromPrefix, toPrefix) || changed;
    }

    if (!changed) {
        return;
    }

    emit dataChanged(index(0), index(m_items.count() - 1), {CurrentPathRole, NewPathRole, ActualPathRole});
}

void RenamePreviewModel::clear()
{
    setItems({});
}

QString RenamePreviewModel::actualPath(const RenamePreviewItem &item) const
{
    if (item.status == QStringLiteral("已转换")) {
        return item.newPath;
    }

    return item.currentPath;
}
