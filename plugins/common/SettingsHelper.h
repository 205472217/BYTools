#pragma once

#include <QSettings>
#include <QString>

/// 返回指定 group 名的 QSettings 引用（INI 文件，缓存，线程安全）
QSettings &pluginGroupSettings(const char *groupName);
