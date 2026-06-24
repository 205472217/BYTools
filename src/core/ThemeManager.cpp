#include "ThemeManager.h"

#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>
#include <QDebug>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    loadPalette();
}

ThemeManager* ThemeManager::instance()
{
    static ThemeManager manager;
    return &manager;
}

QVariantMap ThemeManager::palette() const
{
    return m_palette;
}

QString ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

void ThemeManager::setTheme(const QString &theme)
{
    if (m_currentTheme == theme)
        return;
    m_currentTheme = theme;
    loadPalette();
}

void ThemeManager::loadPalette()
{
    QString fileName = QStringLiteral(":/themes/Theme%1.js").arg(m_currentTheme);
    if (!QFile::exists(fileName)) {
        qWarning() << "ThemeManager: file not found" << fileName;
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ThemeManager: failed to load" << fileName;
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QJSEngine engine;
    QJSValue result = engine.evaluate(content + QStringLiteral("\npalette;"));

    if (result.isError()) {
        qWarning() << "ThemeManager: JS parse error:" << result.toString();
        return;
    }

    if (!result.isObject()) {
        qWarning() << "ThemeManager: palette is not an object";
        return;
    }

    QVariantMap palette;
    QJSValueIterator it(result);
    while (it.hasNext()) {
        it.next();
        palette[it.name()] = it.value().toVariant();
    }

    m_palette = palette;
    emit paletteChanged();
}

QVariantMap ThemeManager::groupPalette(const QString &group) const
{
    QVariantMap result;
    QString prefix = group + "_";
    for (auto it = m_palette.begin(); it != m_palette.end(); ++it)
        if (it.key().startsWith(prefix))
            result[it.key().mid(prefix.length())] = it.value();
    return result;
}

QColor ThemeManager::groupColor(const QString &group, const QString &prop) const
{
    return m_palette.value(group + "_" + prop).value<QColor>();
}
