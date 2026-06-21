#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QDir>

class PluginLogger;
class ImageConverterSettings;

class ImageConverterController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)

public:
    explicit ImageConverterController(PluginLogger *logger, ImageConverterSettings *settings, QObject *parent = nullptr);

    QString statusMessage() const;
    bool hasRecords() const;
    bool isProcessing() const;
    QVariantList records() const;

    Q_INVOKABLE void executeConvert();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void restoreRecord(int index);
    Q_INVOKABLE void restoreAllRecords();
    Q_INVOKABLE void reset();

signals:
    void statusMessageChanged();
    void hasRecordsChanged();
    void recordsChanged();
    void isProcessingChanged();

private:
    struct ConvertRecord {
        QString originalPath;
        QString newPath;
        QString originalName;
        QString newName;
        QString formatTag;
        bool success;
        QString status;
    };

    void processDirectory(const QDir &currentDir, const QString &relativePath,
                          int &successCount, int &failCount, int &skipCount,
                          const QString &rootPath, int targetFormat,
                          int outputMode, const QString &outputDir,
                          int quality, const QString &bgColor, bool recursive);
    void addRecord(const QString &originalPath, const QString &newPath,
                   const QString &formatTag, bool success, const QString &status);
    void setStatusMessage(const QString &message);
    void setIsProcessing(bool processing);

    bool isImageFile(const QString &fileName) const;
    static QString formatExtension(int formatIndex);
    static QByteArray formatForQImage(int formatIndex);
    static QString formatTagForExt(const QString &ext);

    bool m_isProcessing = false;
    QString m_statusMessage;
    QList<ConvertRecord> m_records;

    ImageConverterSettings *m_settings = nullptr;
    PluginLogger *m_logger = nullptr;
    const QStringList m_imageExtensions = {
        ".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tiff", ".tif",
        ".gif", ".ico", ".tga", ".ppm", ".pgm", ".pbm", ".xbm", ".xpm"
    };
};
