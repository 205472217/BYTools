#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QDir>
#include <QThread>
#include <QMutex>

class PluginLogger;
class ImageConverterSettings;

class ImageConverterController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString sourceFile READ sourceFile WRITE setSourceFile NOTIFY sourceFileChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)

    // === Config properties (delegated to ImageConverterSettings) ===
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(int targetFormat READ targetFormat WRITE setTargetFormat NOTIFY targetFormatChanged)
    Q_PROPERTY(int quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(QString bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)
    Q_PROPERTY(int outputMode READ outputMode WRITE setOutputMode NOTIFY outputModeChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(bool convertEnabled READ convertEnabled WRITE setConvertEnabled NOTIFY convertEnabledChanged)
    Q_PROPERTY(bool resizeEnabled READ resizeEnabled WRITE setResizeEnabled NOTIFY resizeEnabledChanged)
    Q_PROPERTY(int resizeMode READ resizeMode WRITE setResizeMode NOTIFY resizeModeChanged)
    Q_PROPERTY(double resizeRatio READ resizeRatio WRITE setResizeRatio NOTIFY resizeRatioChanged)
    Q_PROPERTY(int resizeWidth READ resizeWidth WRITE setResizeWidth NOTIFY resizeWidthChanged)
    Q_PROPERTY(int resizeHeight READ resizeHeight WRITE setResizeHeight NOTIFY resizeHeightChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)

public:
    explicit ImageConverterController(PluginLogger *logger, ImageConverterSettings *settings, QObject *parent = nullptr);
    ~ImageConverterController() override;

    QString statusMessage() const;
    QString sourceFile() const;
    void setSourceFile(const QString &path);
    bool hasRecords() const;
    bool isProcessing() const;
    QVariantList records() const;

    // Config property getters
    QString rootPath() const;
    int targetFormat() const;
    int quality() const;
    QString bgColor() const;
    int outputMode() const;
    QString outputDir() const;
    bool recursive() const;
    bool convertEnabled() const;
    bool resizeEnabled() const;
    int resizeMode() const;
    double resizeRatio() const;
    int resizeWidth() const;
    int resizeHeight() const;
    int mode() const;

    // Config property setters
    void setRootPath(const QString &path);
    void setTargetFormat(int format);
    void setQuality(int quality);
    void setBgColor(const QString &color);
    void setOutputMode(int mode);
    void setOutputDir(const QString &dir);
    void setRecursive(bool recursive);
    void setConvertEnabled(bool enabled);
    void setResizeEnabled(bool enabled);
    void setResizeMode(int mode);
    void setResizeRatio(double ratio);
    void setResizeWidth(int w);
    void setResizeHeight(int h);
    void setMode(int mode);

    Q_INVOKABLE void executeConvert();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void restoreRecord(int index);
    Q_INVOKABLE void restoreAllRecords();
    Q_INVOKABLE void reset();

signals:
    void statusMessageChanged();
    void sourceFileChanged();
    void hasRecordsChanged();
    void recordsChanged();
    void isProcessingChanged();
    void logMessage(const QString &message);

    // Config property signals
    void rootPathChanged();
    void targetFormatChanged();
    void qualityChanged();
    void bgColorChanged();
    void outputModeChanged();
    void outputDirChanged();
    void recursiveChanged();
    void convertEnabledChanged();
    void resizeEnabledChanged();
    void resizeModeChanged();
    void resizeRatioChanged();
    void resizeWidthChanged();
    void resizeHeightChanged();
    void modeChanged();

private:
    struct ConvertRecord {
        QString originalPath;
        QString newPath;
        QString originalName;
        QString newName;
        QString formatTag;
        bool success;
        QString status;
        bool restorable = false;
        QString backupPath;
    };

    void doWork();
    void processSingleFile(const QString &filePath, const QString &destPath,
        bool doConvert, int targetFormat, int quality,
        bool doResize, int resizeMode, double resizeRatio,
        int resizeWidth, int resizeHeight, const QString &bgColor,
        const QString &targetExt, const QByteArray &targetFmt,
        int outputMode, int &successCount, int &failCount, int &skipCount,
        QList<ConvertRecord> &records);
    void addRecord(const QString &originalPath, const QString &newPath,
                   const QString &formatTag, bool success, const QString &status);
    void setStatusMessage(const QString &message);
    void setIsProcessing(bool processing);

    bool isImageFile(const QString &fileName) const;
    static QString createBackup(const QString &filePath);
    static QString formatExtension(int formatIndex);
    static QByteArray formatForQImage(int formatIndex);
    static QString formatTagForExt(const QString &ext);

    mutable QMutex m_recordsMutex;
    bool m_isProcessing = false;
    QString m_statusMessage;
    QString m_sourceFile;
    QList<ConvertRecord> m_records;

    QThread m_workerThread;
    bool m_workerRunning = false;

    ImageConverterSettings *m_settings = nullptr;
    PluginLogger *m_logger = nullptr;
    const QStringList m_imageExtensions = {
        ".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tiff", ".tif",
        ".gif", ".ico", ".tga", ".ppm", ".pgm", ".pbm", ".xbm", ".xpm"
    };
};
