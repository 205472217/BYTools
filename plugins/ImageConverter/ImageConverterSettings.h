#pragma once

#include <QObject>
#include <QString>

class ImageConverterSettings : public QObject
{
    Q_OBJECT

public:
    explicit ImageConverterSettings(QObject *parent = nullptr);

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

    // ── 其他功能函数 ──
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

private:
    QString m_rootPath;
    int m_targetFormat = 1;
    int m_quality = 85;
    QString m_bgColor = "#ffffff";
    int m_outputMode = 0;
    QString m_outputDir;
    bool m_recursive = false;
    bool m_convertEnabled = false;
    bool m_resizeEnabled = false;
    int m_resizeMode = 0;
    double m_resizeRatio = 0.5;
    int m_resizeWidth = 1920;
    int m_resizeHeight = 1080;
    int m_mode = 1;
};
