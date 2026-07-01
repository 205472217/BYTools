#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

class MatchPairModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    struct MatchPair {
        QString videoFile;
        QString subtitleFile;
        int status = 0; // 0=未处理, 1=已导出
    };

    enum Roles {
        VideoFileRole = Qt::UserRole + 1,
        SubtitleFileRole,
        StatusRole,
        VideoDisplayRole,
        SubtitleDisplayRole,
        StatusDisplayRole
    };

    explicit MatchPairModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setPairs(const QList<MatchPair> &pairs);
    void setStatus(int row, int status);
    const MatchPair &at(int row) const;
    void clear();

signals:
    void countChanged();

private:
    QList<MatchPair> m_pairs;
};
