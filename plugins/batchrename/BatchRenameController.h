#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QDir>
#include <QThread>

class PluginLogger;
class BatchRenameSettings;

class BatchRenameController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)

public:
    explicit BatchRenameController(PluginLogger *logger, BatchRenameSettings *settings, QObject *parent = nullptr);
    ~BatchRenameController() override;

    QString statusMessage() const;
    bool hasRecords() const;
    bool isProcessing() const;
    QVariantList records() const;

    Q_INVOKABLE void executeRename();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void restoreRecord(int index);
    Q_INVOKABLE void restoreAllRecords();
    Q_INVOKABLE void reset();

    QString getFileType(const QString &fileName) const;

signals:
    void statusMessageChanged();
    void hasRecordsChanged();
    void recordsChanged();
    void isProcessingChanged();
    void logMessage(const QString &message);

private:
    struct RenameRecord {
        QString originalPath;
        QString newPath;
        QString originalName;
        QString newName;
        QString fileType;
        bool success;
        QString status;
    };

    void setStatusMessage(const QString &message);
    void setIsProcessing(bool processing);
    void doWork();
    void addRecord(const QString &originalPath, const QString &newPath, bool success, const QString &status);
    bool matchesFileType(const QString &fileName, int fileType, const QString &customExtension) const;
    QString generateNewName(int index, const QString &originalName, const QString &extension,
                            int renameMode, const QString &baseName,
                            const QString &searchText, const QString &replaceText) const;
    QString getFileExtension(const QString &fileName) const;

    bool m_isProcessing = false;
    QString m_statusMessage;
    QList<RenameRecord> m_records;

    QThread m_workerThread;
    bool m_workerRunning = false;

    BatchRenameSettings *m_settings = nullptr;
    PluginLogger *m_logger = nullptr;
    const QStringList m_videoExtensions = {".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".ts"};
    const QStringList m_audioExtensions = {".mp3", ".wav", ".flac", ".ogg", ".aac", ".wma"};
    const QStringList m_textExtensions = {".txt", ".md", ".json", ".xml", ".csv", ".log"};
    const QStringList m_imageExtensions = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp"};
};