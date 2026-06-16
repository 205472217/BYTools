#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SubtitleCat 字幕搜索
独立文件，只负责 subtitlecat.com 的搜索逻辑。
修改此站点适配时不会影响其他站点。
使用 urllib + lxml，不依赖 scrapling / curl_cffi。
"""

import re
import sys
import urllib.request
import urllib.error
import urllib.parse
from lxml.html import fromstring


MAX_RESULTS = 100
MAX_RESOLVE = 10

LANG_MAP = {
    "中文": "Chinese",
    "中文繁体": "Chinese",
    "英文": "English",
}

LANG_URL_PATTERNS = {
    "Chinese": ["zh"],
    "English": ["en"],
}


def search(keyword: str, language_filter: str = "") -> list:
    """搜索 subtitlecat.com，返回结果列表。"""
    results = []
    base = "https://www.subtitlecat.com"
    search_url = f"{base}/index.php?search={keyword}"

    _log(f"search_url: {search_url}, language_filter: {language_filter!r}")

    html = _fetch(search_url)
    if html is None:
        return results

    _parse_html(html, results, base, search_url)
    _log(f"parsed {len(results)} results, capping to {MAX_RESULTS}")

    results = results[:MAX_RESULTS]

    if language_filter:
        target_lang = LANG_MAP.get(language_filter)
        if target_lang:
            before = len(results)
            results = [r for r in results if r.get("language") == target_lang]
            _log(f"language filter '{language_filter}'->'{target_lang}': {before} -> {len(results)}")

    _resolve_downloads(results, base, max_items=MAX_RESOLVE)

    return results


def _fetch(url: str) -> str | None:
    """下载页面"""
    req = urllib.request.Request(
        url,
        headers={
            "User-Agent": (
                "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                "AppleWebKit/537.36 (KHTML, like Gecko) "
                "Chrome/120.0.0.0 Safari/537.36"
            ),
        },
    )
    try:
        resp = urllib.request.urlopen(req, timeout=20)
        _log(f"response status: {resp.status}")
        if resp.status != 200:
            return None
        return resp.read().decode("utf-8", errors="replace")
    except Exception as e:
        _log(f"fetch error: {e}")
        return None


def _log(*args):
    """打印调试日志到 stderr"""
    print("[SubtitleCat]", *args, file=sys.stderr, flush=True)


def _parse_html(html: str, results: list, base: str, current_url: str):
    """解析搜索结果页面的 HTML 表格"""
    _log("page body length:", len(html))

    doc = fromstring(html)
    rows = doc.xpath('//table[contains(@class, "sub-table")]/tbody/tr')
    _log(f"xpath found {len(rows)} rows")

    for row in rows:
        try:
            tds = row.xpath("td")
            if len(tds) < 3:
                continue

            # 列0: 字幕链接 <a>
            links = tds[0].xpath(".//a")
            if not links:
                continue
            a = links[0]
            href = a.get("href", "")
            sub_name = (a.text_content() or "").strip()
            if not href or not sub_name:
                continue
            if not href.startswith("http"):
                href = urllib.parse.urljoin(current_url, href)

            # 从整列文本提取语言
            col_text = tds[0].text_content() or ""
            m = re.search(r"translated from (\w+)", col_text)
            language = m.group(1) if m else "Unknown"

            results.append({
                "site": "SubtitleCat",
                "language": language,
                "file_name": sub_name,
                "download_url": href,
            })
        except Exception:
            continue


def _resolve_downloads(results: list, base: str, max_items=0):
    """访问详情页，查找真正的 .srt / .ass 下载链接
    max_items: 最多解析前 N 条，0 表示全部
    """
    todo = results[:max_items] if max_items > 0 else results
    for item in todo:
        url = item.get("download_url")
        if not url:
            continue
        try:
            html = _fetch(url)
            if html is None:
                continue
            doc = fromstring(html)
            links = doc.xpath(
                "//a[contains(@href, '.srt')] | "
                "//a[contains(@href, '.ass')] | "
                "//a[contains(@href, '.zip')] | "
                "//a[contains(@href, '.rar')] | "
                "//a[contains(@href, 'download.php')]"
            )
            if not links:
                continue

            # 按语言偏好选择下载链接
            item_lang = item.get("language", "")
            patterns = LANG_URL_PATTERNS.get(item_lang, [])
            chosen = links[0]
            if patterns:
                for a in links:
                    href = a.get("href", "")
                    if any(p in href for p in patterns):
                        chosen = a
                        break

            href = chosen.get("href", "")
            if href and not href.startswith("http"):
                href = urllib.parse.urljoin(url, href)
            item["download_url"] = href

            # 从 URL 路径提取带扩展名的文件名
            url_filename = href.split("/")[-1].split("?")[0]
            if "." in url_filename:
                item["file_name"] = url_filename
        except Exception:
            continue
