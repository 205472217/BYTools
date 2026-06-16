#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
字幕文件下载脚本
从 stdin 读取 JSON 请求，下载字幕文件到指定目录，结果输出到 stdout。

请求格式 (stdin JSON):
  {"url": "https://...", "file_name": "xxx.srt", "output_dir": "/path/to/save", "language": "Chinese"}

响应格式 (stdout JSON):
  {"ok": true, "file_path": "/path/to/save/xxx.srt", "size": 12345}
   或
  {"ok": false, "error": "错误描述"}
"""

import sys
import json
import os
import re
import traceback
import urllib.request
import urllib.parse


LANG_URL_PATTERNS = {
    "Chinese": ["zh"],
    "English": ["en"],
}

VALID_EXTS = {".srt", ".ass", ".sub", ".txt"}


def _ensure_extension(file_name: str, url: str) -> str:
    """确保文件名有正确的字幕文件扩展名"""
    _, ext = os.path.splitext(file_name)
    if ext.lower() in VALID_EXTS:
        return file_name
    url_path = url.split("?")[0]
    _, url_ext = os.path.splitext(url_path)
    if url_ext.lower() in VALID_EXTS:
        return file_name + url_ext
    return file_name + ".srt"


def _to_utf8_bytes(data: bytes) -> bytes:
    """确保内容是 UTF-8 编码的字节"""
    try:
        data.decode("utf-8")
        return data
    except UnicodeDecodeError:
        for enc in ("gb18030", "big5", "shift_jis", "gbk"):
            try:
                text = data.decode(enc)
                return text.encode("utf-8")
            except UnicodeDecodeError:
                continue
    return data


def _extract_subtitle_text(html: str) -> str | None:
    """尝试从详情页 HTML 中提取嵌入的字幕文本

    用户在详情页底部可以看到类似：
        1
        00:00:00,407 --> 00:00:03,589
        （字幕文本）
    """
    markers = [
        "Would you like to inspect the original subtitles",
        '<xmp id="subtitles"',
        '<pre id="subtitles"',
    ]
    for marker in markers:
        idx = html.find(marker)
        if idx < 0:
            continue
        rest = html[idx + len(marker):]
        m = re.search(r'(?:\n|^)(1\n\d{2}:\d{2}:\d{2}[,\\.]\d+)', rest)
        if m:
            start = rest.find(m.group(1))
            if start >= 0:
                content_start = idx + len(marker) + start
                end = len(html)
                for tag in ("<script", "</div>", "<br"):
                    pos = html.find(tag, content_start)
                    if 0 < pos < end:
                        end = pos
                result = html[content_start:end].strip()
                if result:
                    return result
    return None


def _resolve_and_download(url: str, file_name: str, output_dir: str, language: str = "") -> dict:
    """下载 URL，如果返回 HTML 则解析出 .srt 链接再下载"""
    req = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                      "AppleWebKit/537.36 (KHTML, like Gecko) "
                      "Chrome/120.0.0.0 Safari/537.36"
    })
    with urllib.request.urlopen(req, timeout=30) as resp:
        body = resp.read()
        content_type = resp.headers.get("Content-Type", "")

    is_html = "text/html" in content_type or body[:200].strip().lower().startswith(b"<")

    if is_html:
        from lxml.html import fromstring
        html_text = body.decode("utf-8", errors="replace")
        doc = fromstring(html_text)

        links = doc.xpath("//a[contains(@href, '.srt')] | //a[contains(@href, '.ass')]")
        if links:
            chosen = links[0]
            if language:
                patterns = LANG_URL_PATTERNS.get(language, [])
                for a in links:
                    href = a.get("href", "")
                    if any(p in href for p in patterns):
                        chosen = a
                        break
            href = chosen.get("href", "")
            if href:
                if not href.startswith("http"):
                    href = urllib.parse.urljoin(url, href)
                srt_name = href.split("/")[-1].split("?")[0]
                if "." in srt_name:
                    file_name = srt_name
                return _resolve_and_download(href, file_name, output_dir, language)

        text = _extract_subtitle_text(html_text)
        if text:
            os.makedirs(output_dir, exist_ok=True)
            file_name = _ensure_extension(file_name, url)
            save_path = os.path.join(output_dir, file_name)
            data = text.encode("utf-8")
            with open(save_path, "wb") as f:
                f.write(data)
            return {"ok": True, "file_path": save_path, "size": len(data)}

        return {"ok": False, "error": "详情页中未找到字幕下载链接"}

    os.makedirs(output_dir, exist_ok=True)
    file_name = _ensure_extension(file_name, url)
    save_path = os.path.join(output_dir, file_name)
    data = _to_utf8_bytes(body)
    with open(save_path, "wb") as f:
        f.write(data)
    return {"ok": True, "file_path": save_path, "size": len(data)}


def _write_json(obj):
    """输出 JSON 到 stdout，绕过控制台编码限制"""
    raw = json.dumps(obj, ensure_ascii=True)
    sys.stdout.buffer.write(raw.encode('utf-8'))
    sys.stdout.buffer.write(b'\n')
    sys.stdout.buffer.flush()


def main():
    try:
        raw = sys.stdin.read()
        req = json.loads(raw)
    except (json.JSONDecodeError, Exception) as e:
        _write_json({"ok": False, "error": f"请求解析失败: {e}"})
        return

    url = req.get("url", "").strip()
    file_name = req.get("file_name", "").strip()
    output_dir = req.get("output_dir", "").strip()
    language = req.get("language", "").strip()

    if not url:
        _write_json({"ok": False, "error": "下载 URL 不能为空"})
        return
    if not output_dir:
        _write_json({"ok": False, "error": "输出目录不能为空"})
        return
    if not file_name:
        file_name = url.split("/")[-1].split("?")[0]
        if not file_name:
            file_name = "subtitle.srt"

    try:
        result = _resolve_and_download(url, file_name, output_dir, language)
        _write_json(result)
    except Exception as e:
        tb = traceback.format_exc()
        _write_json({"ok": False, "error": str(e), "traceback": tb})


if __name__ == "__main__":
    main()