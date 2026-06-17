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
import os
import urllib.request
import urllib.error
import urllib.parse
from lxml.html import fromstring

_parent = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _parent not in sys.path:
    sys.path.insert(0, _parent)
import log_util


LANG_URL_PATTERNS = {
    "Chinese": ["zh"],
    "English": ["en"],
}


def search(keyword: str, language_filter: str = "", max_results: int = 50) -> list:
    """搜索 subtitlecat.com，返回结果列表。
    language_filter: 由 C++ 传过来的 pageLabel（如 "Chinese (Simplified)"），
    在详情页匹配 Download 按钮前的文本。
    """
    results = []
    base = "https://www.subtitlecat.com"
    search_url = f"{base}/index.php?search={keyword}"

    log_util.log("[SubtitleCat]", f"search_url: {search_url}, language_filter: {language_filter!r}")

    html = _fetch(search_url)
    if html is None:
        return results

    _parse_html(html, results, base, search_url)
    log_util.log("[SubtitleCat]", f"parsed {len(results)} results, capping to {max_results}")

    results = results[:max_results]

    if language_filter:
        # 有语言筛选：进详情页，按 Download 按钮前的文本匹配
        log_util.log("[SubtitleCat]", f"filtering by download state: target_label={language_filter}")
        return _filter_by_download_state(results, base, language_filter, max_items=max_results)
    else:
        # 无筛选（兜底）：取第一个下载链接
        _resolve_downloads(results, base, max_items=max_results)
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
        log_util.log("[SubtitleCat]", f"response status: {resp.status}")
        if resp.status != 200:
            log_util.log("[SubtitleCat]", f"非200响应: {resp.status}")
            return None
        return resp.read().decode("utf-8", errors="replace")
    except Exception as e:
        log_util.log("[SubtitleCat]", f"fetch error: {e}")
        return None


def _parse_html(html: str, results: list, base: str, current_url: str):
    """解析搜索结果页面的 HTML 表格"""
    log_util.log("[SubtitleCat]", "page body length:", len(html))

    doc = fromstring(html)
    rows = doc.xpath('//table[contains(@class, "sub-table")]/tbody/tr')
    log_util.log("[SubtitleCat]", f"xpath found {len(rows)} rows")

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
    """无语言筛选时：进详情页，找第一个 green-link 作为下载链接"""
    todo = results[:max_items] if max_items > 0 else results
    total = len(todo)
    for idx, item in enumerate(todo):
        log_util.write_progress(idx + 1, total, f"正在解析第 {idx+1}/{total} 条...")
        url = item.get("download_url")
        if not url:
            continue
        try:
            html = _fetch(url)
            if html is None:
                continue
            doc = fromstring(html)
            links = doc.xpath('.//a[contains(@class, "green-link")]')
            if not links:
                continue

            href = links[0].get("href", "")
            if not href:
                continue
            if not href.startswith("http"):
                href = urllib.parse.urljoin(base, href)
            item["download_url"] = href
            url_filename = href.split("/")[-1].split("?")[0]
            if "." in url_filename:
                item["file_name"] = url_filename
        except Exception:
            continue


def _filter_by_download_state(results, base, target_page_label, max_items=0):
    """有语言筛选时：进详情页，找到目标语种文本，在附近找 Download 链接。
    不做任何结构假设。
    """
    filtered = []
    todo = results[:max_items] if max_items > 0 else results
    total = len(todo)
    for idx, item in enumerate(todo):
        log_util.write_progress(idx + 1, total, f"正在分析第 {idx+1}/{total} 条...")
        url = item.get("download_url")
        if not url:
            continue
        try:
            html = _fetch(url)
            if html is None:
                continue
            doc = fromstring(html)
            href = _find_download_near_text(doc, target_page_label, base)
            if not href:
                log_util.log("[SubtitleCat]", f"  skip {url}: no download near '{target_page_label}'")
                continue

            item["download_url"] = href
            url_filename = href.split("/")[-1].split("?")[0]
            if "." in url_filename:
                item["file_name"] = url_filename
            log_util.log("[SubtitleCat]", f"  OK {href}")
            filtered.append(item)
        except Exception:
            continue

    log_util.log("[SubtitleCat]", f"_filter_by_download_state({target_page_label}): {len(todo)} -> {len(filtered)}")
    return filtered


def _find_download_near_text(doc, target_text, base):
    """找到目标文本，上到最近的 <div>，在 div 范围内找 green-link"""
    for elem in doc.iter():
        if elem.tag in ('script', 'style', 'head', 'meta'):
            continue
        text = (elem.text or "").strip()
        if text != target_text:
            continue
        # 上到最近的 <div>
        div = elem
        while div.tag != 'div' and div.getparent() is not None:
            div = div.getparent()
        if div.tag != 'div':
            continue
        # 在这个 div 里找 green-link
        links = div.xpath('.//a[contains(@class, "green-link")]')
        if not links:
            continue
        href = links[0].get("href", "")
        if not href:
            continue
        if not href.startswith("http"):
            href = urllib.parse.urljoin(base, href)
        return href
    return ""
