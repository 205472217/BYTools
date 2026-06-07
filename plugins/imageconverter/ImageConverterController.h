#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QDir>

class ImageConverterController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(int targetFormat READ targetFormat WRITE setTargetFormat NOTIFY targetFormatChanged)
    Q_PROPERTY(int quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(QString bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)
    Q_PROPERTY(int outputMode READ outputMode WRITE setOutputMode NOTIFY outputModeChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)

public:
    explicit ImageConverterController(QObject *parent = nullptr);

    QString rootPath() const;
    void setRootPath(const QString &rootPath);

    int targetFormat() const;
    void setTargetFormat(int format);

    int quality() const;
    void setQuality(int quality);

    QString bgColor() const;
    void setBgColor(const QString &color);

    int outputMode() const;
    void setOutputMode(int mode);

    QString outputDir() const;
    void setOutputDir(const QString &dir);

    bool recursive() const;
    void setRecursive(bool recursive);

    QString statusMessage() const;
    bool hasRecords() const;
    bool isProcessing() const;
    QVariantList records() const;

    Q_INVOKABLE void executeConvert();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void reset();

signals:
    void rootPathChanged();
    void targetFormatChanged();
    void qualityChanged();
    void bgColorChanged();
    void outputModeChanged();
    void outputDirChanged();
    void recursiveChanged();
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
                          int &successCount, int &failCount, int &skipCount);
    void addRecord(const QString &originalPath, const QString &newPath,
                   const QString &formatTag, bool success, const QString &status);
    void setStatusMessage(const QString &message);
    void setIsProcessing(bool processing);

    bool isImageFile(const QString &fileName) const;
    static QString formatExtension(int formatIndex);
    static QByteArray formatForQImage(int formatIndex);
    static QString formatTagForExt(const QString &ext);

    QString m_rootPath;
    int m_targetFormat = 1;  // 0=PNG, 1=JPG, 2=BMP, 3=WebP, 4=TIFF
    int m_quality = 85;
    QString m_bgColor = "#ffffff";
    int m_outputMode = 0;   // 0=替换原文件, 1=输出到新目录
    QString m_outputDir;
    bool m_recursive = false;
    bool m_isProcessing = false;
    QString m_statusMessage;
    QList<ConvertRecord> m_records;

    const QStringList m_imageExtensions = {
        ".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tiff", ".tif",
        ".gif", ".ico", ".tga", ".ppm", ".pgm", ".pbm", ".xbm", ".xpm"
    };
};
