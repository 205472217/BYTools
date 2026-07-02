#pragma once

#include <QObject>
#include <QString>

class NameConverterSettings : public QObject
{
    Q_OBJECT

public:
    explicit NameConverterSettings(QObject *parent = nullptr);

    QString rootPath() const;
    int targetType() const;
    bool recursive() const;

    void setRootPath(const QString &path);
    void setTargetType(int type);
    void setRecursive(bool recursive);

    // ── 其他功能函数 ──
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

private:
    QString m_rootPath;
    int m_targetType = 2;
    bool m_recursive = false;
};
