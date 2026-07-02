#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class PluginLogger;

class ImageCropSettings : public QObject
{
    Q_OBJECT

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

    // ── 其他功能函数 ──
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

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
