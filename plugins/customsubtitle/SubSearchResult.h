#pragma once

#include <QString>

/// 字幕搜索结果条目
struct SubSearchResult
{
    QString site;
    QString language;
    QString fileName;
    QString downloadUrl;

    SubSearchResult() = default;
    SubSearchResult(const QString &site, const QString &language,
                    const QString &fileName, const QString &url)
        : site(site), language(language), fileName(fileName), downloadUrl(url) {}
};