#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QDir>
#include <QImage>>

class PluginLogger;
class ImageCropSettings;

class ImageCropController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cropX READ cropX WRITE setCropX NOTIFY cropXChanged)
    Q_PROPERTY(int cropY READ cropY WRITE setCropY NOTIFY cropYChanged)
    Q_PROPERTY(int cropW READ cropW WRITE setCropW NOTIFY cropWChanged)
    Q_PROPERTY(int cropH READ cropH WRITE setCropH NOTIFY cropHChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(int currentFileCount READ currentFileCount NOTIFY currentFileCountChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentFilePathChanged)
    Q_PROPERTY(int imageVersion READ imageVersion NOTIFY imageVersionChanged)
    Q_PROPERTY(bool canRestoreCurrent READ canRestoreCurrent NOTIFY canRestoreCurrentChanged)

public:
    explicit ImageCropController(PluginLogger *logger, ImageCropSettings *settings, QObject *parent = nullptr);

    int cropX() const;
    void setCropX(int x);

    int cropY() const;
    void setCropY(int y);

    int cropW() const;
    void setCropW(int w);

    int cropH() const;
    void setCropH(int h);

    QString statusMessage() const;
    bool hasRecords() const;
    QVariantList records() const;

    int currentIndex() const;
    int currentFileCount() const;
    QString currentFilePath() const;
    bool isProcessing() const;
    int imageVersion() const;
    bool canRestoreCurrent() const;

    Q_INVOKABLE void scanImages();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE bool navigateNext();
    Q_INVOKABLE bool navigatePrev();
    Q_INVOKABLE QVariantMap getCurrentImageInfo() const;
    Q_INVOKABLE QVariantList getAllFilePaths() const;
    Q_INVOKABLE int getFileCount() const;
    Q_INVOKABLE bool executeCrop(int cropX, int cropY, int cropW, int cropH);
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void restoreCurrentFile();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void resetCropRect();

    // Crop calculation helpers (moved from QML business logic)
    Q_INVOKABLE double calcEffectiveRatio() const;
    Q_INVOKABLE QVariantMap constrainToRatio(double rawW, double rawH) const;
    Q_INVOKABLE QVariantMap calcDisplayDimensions(int containerW, int containerH, int srcW, int srcH);
    Q_INVOKABLE QVariantMap calcInitCropRect(int dispW, int dispH, int srcW, int srcH) const;
    Q_INVOKABLE QVariantMap clampCropRect(int cropX, int cropY, int cropW, int cropH, int dispW, int dispH);
    Q_INVOKABLE void syncCropToController(int cropX, int cropY, int cropW, int cropH, int dispW, int dispH, int srcW, int srcH);
    Q_INVOKABLE int hitTest(int mx, int my, int cropX, int cropY, int cropW, int cropH, int cornerHitSize) const;
    Q_INVOKABLE static QString extractFileName(const QString &filePath);

signals:
    void cropXChanged();
    void cropYChanged();
    void cropWChanged();
    void cropHChanged();
    void statusMessageChanged();
    void hasRecordsChanged();
    void recordsChanged();
    void currentIndexChanged();
    void currentFileCountChanged();
    void currentFilePathChanged();
    void isProcessingChanged();
    void imageVersionChanged();
    void canRestoreCurrentChanged();

private:
    struct CropRecord {
        QString originalPath;
        QString newPath;
        QString originalName;
        QString newName;
        int cropW;
        int cropH;
        bool success;
        QString status;
    };

    void setStatusMessage(const QString &message);
    void setIsProcessing(bool processing);
    bool isImageFile(const QString &fileName) const;
    void addRecord(const QString &originalPath, const QString &newPath,
                   int cropW, int cropH, bool success, const QString &status);
    QString buildCropSuffix() const;

    bool m_isProcessing = false;
    int m_imageVersion = 0;
    int m_cropX = 0;
    int m_cropY = 0;
    int m_cropW = 0;
    int m_cropH = 0;
    QString m_statusMessage;

    QStringList m_imageFiles;
    int m_currentIndex = -1;

    QList<CropRecord> m_records;

    QList<QImage> m_backups;
    static constexpr int MAX_BACKUPS = 10;

    static constexpr int RATIO_COUNT = 4;
    static constexpr double PRESET_RATIOS[RATIO_COUNT][2] = {
        {1, 1}, {4, 3}, {16, 9}, {9, 16}
    };

    ImageCropSettings *m_settings = nullptr;
    PluginLogger *m_logger = nullptr;
    const QStringList m_imageExtensions = {
        ".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tiff", ".gif"
    };
};
