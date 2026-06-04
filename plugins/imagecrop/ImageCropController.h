#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QDir>

class ImageCropController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(int cropMode READ cropMode WRITE setCropMode NOTIFY cropModeChanged)
    Q_PROPERTY(int presetRatioIndex READ presetRatioIndex WRITE setPresetRatioIndex NOTIFY presetRatioIndexChanged)
    Q_PROPERTY(bool usePresetRatio READ usePresetRatio WRITE setUsePresetRatio NOTIFY usePresetRatioChanged)
    Q_PROPERTY(int customRatioW READ customRatioW WRITE setCustomRatioW NOTIFY customRatioWChanged)
    Q_PROPERTY(int customRatioH READ customRatioH WRITE setCustomRatioH NOTIFY customRatioHChanged)
    Q_PROPERTY(int targetWidth READ targetWidth WRITE setTargetWidth NOTIFY targetWidthChanged)
    Q_PROPERTY(int targetHeight READ targetHeight WRITE setTargetHeight NOTIFY targetHeightChanged)
    Q_PROPERTY(int cropX READ cropX WRITE setCropX NOTIFY cropXChanged)
    Q_PROPERTY(int cropY READ cropY WRITE setCropY NOTIFY cropYChanged)
    Q_PROPERTY(int cropW READ cropW WRITE setCropW NOTIFY cropWChanged)
    Q_PROPERTY(int cropH READ cropH WRITE setCropH NOTIFY cropHChanged)
    Q_PROPERTY(int outputMode READ outputMode WRITE setOutputMode NOTIFY outputModeChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix NOTIFY suffixChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int currentFileCount READ currentFileCount NOTIFY currentFileCountChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentFilePathChanged)

public:
    explicit ImageCropController(QObject *parent = nullptr);

    QString rootPath() const;
    void setRootPath(const QString &path);

    bool recursive() const;
    void setRecursive(bool recursive);

    int cropMode() const;
    void setCropMode(int mode);

    int presetRatioIndex() const;
    void setPresetRatioIndex(int index);

    bool usePresetRatio() const;
    void setUsePresetRatio(bool use);

    int customRatioW() const;
    void setCustomRatioW(int w);

    int customRatioH() const;
    void setCustomRatioH(int h);

    int targetWidth() const;
    void setTargetWidth(int w);

    int targetHeight() const;
    void setTargetHeight(int h);

    int cropX() const;
    void setCropX(int x);

    int cropY() const;
    void setCropY(int y);

    int cropW() const;
    void setCropW(int w);

    int cropH() const;
    void setCropH(int h);

    int outputMode() const;
    void setOutputMode(int mode);

    QString outputDir() const;
    void setOutputDir(const QString &dir);

    QString suffix() const;
    void setSuffix(const QString &suffix);

    QString statusMessage() const;
    bool hasRecords() const;
    QVariantList records() const;

    int currentIndex() const;
    int currentFileCount() const;
    QString currentFilePath() const;

    Q_INVOKABLE void scanImages();
    Q_INVOKABLE bool navigateNext();
    Q_INVOKABLE bool navigatePrev();
    Q_INVOKABLE QVariantMap getCurrentImageInfo() const;
    Q_INVOKABLE QVariantList getAllFilePaths() const;
    Q_INVOKABLE int getFileCount() const;
    Q_INVOKABLE bool executeCrop(int cropX, int cropY, int cropW, int cropH);
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void reset();

signals:
    void rootPathChanged();
    void recursiveChanged();
    void cropModeChanged();
    void presetRatioIndexChanged();
    void usePresetRatioChanged();
    void customRatioWChanged();
    void customRatioHChanged();
    void targetWidthChanged();
    void targetHeightChanged();
    void cropXChanged();
    void cropYChanged();
    void cropWChanged();
    void cropHChanged();
    void outputModeChanged();
    void outputDirChanged();
    void suffixChanged();
    void statusMessageChanged();
    void hasRecordsChanged();
    void recordsChanged();
    void currentIndexChanged();
    void currentFileCountChanged();
    void currentFilePathChanged();

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
    bool isImageFile(const QString &fileName) const;
    void addRecord(const QString &originalPath, const QString &newPath,
                   int cropW, int cropH, bool success, const QString &status);
    QString buildCropSuffix() const;

    QString m_rootPath;
    bool m_recursive = false;
    int m_cropMode = 0;            // 0=ratio, 1=pixel
    int m_presetRatioIndex = 0;    // 0=1:1, 1=4:3, 2=3:2, 3=16:9, 4=9:16, 5=21:9
    bool m_usePresetRatio = true;
    int m_customRatioW = 1;
    int m_customRatioH = 1;
    int m_targetWidth = 800;
    int m_targetHeight = 600;
    int m_cropX = 0;
    int m_cropY = 0;
    int m_cropW = 0;
    int m_cropH = 0;
    int m_outputMode = 0;          // 0=overwrite, 1=new dir
    QString m_outputDir;
    QString m_suffix = "_cropped";
    QString m_statusMessage;

    QStringList m_imageFiles;
    int m_currentIndex = -1;

    QList<CropRecord> m_records;

    static constexpr int RATIO_COUNT = 6;
    static constexpr double PRESET_RATIOS[RATIO_COUNT][2] = {
        {1, 1}, {4, 3}, {3, 2}, {16, 9}, {9, 16}, {21, 9}
    };

    const QStringList m_imageExtensions = {
        ".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tiff", ".gif"
    };
};