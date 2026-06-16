#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SubtitleCat 字幕搜索
独立文件，只负责 subtitlecat.com 的搜索逻辑。
修改此站点适配时不会影响其他站点。
"""


def search(keyword: str) -> list:
    """搜索 subtitlecat.com，返回结果列表。"""
    try:
        from scrapling.fetchers import FetcherSession
    except ImportError:
        return _search_single(keyword)

    return _search_impl(keyword, use_session=True)


def _search_single(keyword: str) -> list:
    """使用 Fetcher 单次请求模式（回退方案）"""
    from scrapling.fetchers import Fetcher
    return _search_impl(keyword, fetcher=Fetcher)


def _search_impl(keyword: str, use_session=False, fetcher=None) -> list:
    """搜索实现（FetcherSession 或 Fetcher 两种模式共用）"""
    results = []
    base = "https://www.subtitlecat.com"
    search_url = f"{base}/index.php?search={keyword}"

    if use_session:
        from scrapling.fetchers import FetcherSession
        with FetcherSession(impersonate="chrome") as session:
            page = session.get(search_url, stealthy_headers=True, timeout=20)
            if page.status != 200:
                return results
            _parse_page(page, results, base)
            _resolve_download_urls(results, session, base)
    else:
        page = fetcher.get(search_url, stealthy_headers=True, timeout=20)
        if page.status != 200:
            return results
        _parse_page(page, results, base)
        _resolve_download_urls(results, fetcher, base)

    return results


def _parse_page(page, results: list, base: str):
    """解析搜索结果页面的 HTML 表格"""
    rows = page.css("table.table.sub-table tbody tr")
    for row in rows:
        try:
            cols = row.css("td")
            if len(cols) < 3:
                continue

            # 列0: 语言
            lang_els = cols[0].css("a[href*='language']::text, a::text")
            language = lang_els.get("").strip() if lang_els else "Unknown"

            # 遍历其他列查找字幕链接
            sub_name = ""
            detail_url = ""
            for ci, col in enumerate(cols):
                if ci == 0:
                    continue
                links = col.css("a[href*='detail'], a[href*='download'], a")
                for link in links:
                    href = link.attrib.get("href", "")
                    text = (link.text or "").strip()
                    if href and not href.startswith("http"):
                        href = base + href
                    if href and text and not sub_name:
                        sub_name = text
                        detail_url = href

            if not sub_name or not detail_url:
                continue

            results.append({
                "site": "SubtitleCat",
                "language": language,
                "file_name": sub_name,
                "download_url": detail_url,
            })
        except Exception:
            continue


def _resolve_download_urls(results: list, fetcher_or_session, base: str):
    """访问详情页，查找真正的文件下载链接"""
    for item in results:
        if item["download_url"]:
            try:
                detail = fetcher_or_session.get(
                    item["download_url"], stealthy_headers=True, timeout=15
                )
                if detail.status == 200:
                    dl_link = detail.css(
                        "a[href*='.srt'], a[href*='.zip'], a[href*='.rar'], "
                        "a[href*='download.php']"
                    )
                    if dl_link:
                        href = dl_link[0].attrib.get("href", "")
                        if href and not href.startswith("http"):
                            href = base + href
                        item["download_url"] = href
            except Exception:
                pass
