#pragma once

#include <QObject>
#include <QString>

class NameConverterSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(int targetType READ targetType WRITE setTargetType NOTIFY targetTypeChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)

public:
    explicit NameConverterSettings(QObject *parent = nullptr);

    QString rootPath() const;
    int targetType() const;
    bool recursive() const;

    void setRootPath(const QString &path);
    void setTargetType(int type);
    void setRecursive(bool recursive);

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

signals:
    void rootPathChanged();
    void targetTypeChanged();
    void recursiveChanged();

private:
    QString m_rootPath;
    int m_targetType = 2;
    bool m_recursive = false;
};
