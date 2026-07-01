#pragma once

#include <QObject>
#include <QString>

class ImageConverterSettings : public QObject
{
    Q_OBJECT
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

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

signals:
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
    QString m_rootPath;
    int m_targetFormat = 1;
    int m_quality = 85;
    QString m_bgColor = "#ffffff";
    int m_outputMode = 0;
    QString m_outputDir;
    bool m_recursive = false;
    bool m_convertEnabled = true;
    bool m_resizeEnabled = false;
    int m_resizeMode = 0;
    double m_resizeRatio = 0.5;
    int m_resizeWidth = 1920;
    int m_resizeHeight = 1080;
    int m_mode = 1;
};
