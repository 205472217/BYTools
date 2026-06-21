#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class PluginLogger;

class ImageCropSettings : public QObject
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
    Q_PROPERTY(int outputMode READ outputMode WRITE setOutputMode NOTIFY outputModeChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)

public:
    explicit ImageCropSettings(QObject *parent = nullptr);

    QString rootPath() const;
    bool recursive() const;
    int cropMode() const;
    int presetRatioIndex() const;
    bool usePresetRatio() const;
    int customRatioW() const;
    int customRatioH() const;
    int targetWidth() const;
    int targetHeight() const;
    int outputMode() const;
    QString outputDir() const;

    void setRootPath(const QString &path);
    void setRecursive(bool recursive);
    void setCropMode(int mode);
    void setPresetRatioIndex(int index);
    void setUsePresetRatio(bool use);
    void setCustomRatioW(int w);
    void setCustomRatioH(int h);
    void setTargetWidth(int w);
    void setTargetHeight(int h);
    void setOutputMode(int mode);
    void setOutputDir(const QString &dir);

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

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
    void outputModeChanged();
    void outputDirChanged();

private:
    QString m_rootPath;
    bool m_recursive = false;
    int m_cropMode = 0;
    int m_presetRatioIndex = 0;
    bool m_usePresetRatio = true;
    int m_customRatioW = 1;
    int m_customRatioH = 1;
    int m_targetWidth = 800;
    int m_targetHeight = 600;
    int m_outputMode = 0;
    QString m_outputDir;
};
