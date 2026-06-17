#include "MatchPairModel.h"
#include <QFileInfo>

MatchPairModel::MatchPairModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MatchPairModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_pairs.size();
}

QVariant MatchPairModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_pairs.size())
        return {};

    const auto &pair = m_pairs.at(index.row());

    switch (role) {
    case VideoFileRole:
        return pair.videoFile;
    case SubtitleFileRole:
        return pair.subtitleFile;
    case StatusRole:
        return pair.status;
    case VideoDisplayRole:
        return QFileInfo(pair.videoFile).fileName();
    case SubtitleDisplayRole:
        return QFileInfo(pair.subtitleFile).fileName();
    case StatusDisplayRole:
        return pair.status == 1 ? QStringLiteral("已导出") : QStringLiteral("未处理");
    }
    return {};
}

QHash<int, QByteArray> MatchPairModel::roleNames() const
{
    return {
        { VideoFileRole,        "videoFile" },
        { SubtitleFileRole,     "subtitleFile" },
        { StatusRole,           "status" },
        { VideoDisplayRole,     "videoDisplay" },
        { SubtitleDisplayRole,  "subtitleDisplay" },
        { StatusDisplayRole,    "statusDisplay" },
    };
}

void MatchPairModel::setPairs(const QList<MatchPair> &pairs)
{
    beginResetModel();
    m_pairs = pairs;
    endResetModel();
    emit countChanged();
}

void MatchPairModel::setStatus(int row, int status)
{
    if (row < 0 || row >= m_pairs.size())
        return;
    m_pairs[row].status = status;
    emit dataChanged(index(row), index(row), { StatusRole, StatusDisplayRole });
}

const MatchPairModel::MatchPair &MatchPairModel::at(int row) const
{
    return m_pairs.at(row);
}

void MatchPairModel::clear()
{
    beginResetModel();
    m_pairs.clear();
    endResetModel();
    emit countChanged();
}
