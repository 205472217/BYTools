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

public:
    explicit ImageConverterSettings(QObject *parent = nullptr);

    QString rootPath() const;
    int targetFormat() const;
    int quality() const;
    QString bgColor() const;
    int outputMode() const;
    QString outputDir() const;
    bool recursive() const;

    void setRootPath(const QString &path);
    void setTargetFormat(int format);
    void setQuality(int quality);
    void setBgColor(const QString &color);
    void setOutputMode(int mode);
    void setOutputDir(const QString &dir);
    void setRecursive(bool recursive);

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

private:
    QString m_rootPath;
    int m_targetFormat = 1;
    int m_quality = 85;
    QString m_bgColor = "#ffffff";
    int m_outputMode = 0;
    QString m_outputDir;
    bool m_recursive = false;
};
