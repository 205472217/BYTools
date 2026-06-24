#pragma once

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QColor>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap palette READ palette NOTIFY paletteChanged)
    Q_PROPERTY(QString currentTheme READ currentTheme WRITE setTheme NOTIFY paletteChanged)

public:
    static ThemeManager* instance();

    QVariantMap palette() const;
    QString currentTheme() const;

    Q_INVOKABLE void setTheme(const QString &theme);
    Q_INVOKABLE QVariantMap groupPalette(const QString &group) const;
    Q_INVOKABLE QColor groupColor(const QString &group, const QString &prop) const;

signals:
    void paletteChanged();

private:
    ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() = default;

    QString m_currentTheme = "Light";
    QVariantMap m_palette;

    void loadPalette();
};
