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
from urllib.parse import urlparse, urlunparse, quote
import log_util


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


def _encode_url(url: str) -> str:
    """确保 URL 中的特殊字符被正确编码（如空格 → %20），避免 urllib 请求失败

    不会对已有 % 编码造成二次编码（% 在 safe 中）。
    """
    parsed = urlparse(url)
    # 编码路径（保留 / 和已有 % 编码）
    path = quote(parsed.path, safe='/%')
    # 编码查询参数（保留 = & 和已有 % 编码）
    query = quote(parsed.query, safe='=&')
    # 编码 fragment（保留已有 % 编码）
    fragment = quote(parsed.fragment, safe='/%')
    return urlunparse((parsed.scheme, parsed.netloc, path, parsed.params, query, fragment))


def _resolve_and_download(url: str, file_name: str, output_dir: str, language: str = "") -> dict:
    """下载 URL（已经是最终下载链接），直接 GET 保存"""
    url = _encode_url(url)
    log_util.log(f"[Download] 发起 HTTP 请求: {url}")
    req = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                      "AppleWebKit/537.36 (KHTML, like Gecko) "
                      "Chrome/120.0.0.0 Safari/537.36",
        "Referer": "https://www.subtitlecat.com/",
    })
    with urllib.request.urlopen(req, timeout=30) as resp:
        log_util.log(f"[Download] HTTP 响应: {resp.status}")
        body = resp.read()

    os.makedirs(output_dir, exist_ok=True)
    file_name = _ensure_extension(file_name, url)
    save_path = os.path.join(output_dir, file_name).replace("\\", "/")
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
    here = os.path.dirname(os.path.abspath(__file__))
    log_util.init_log(here)

    try:
        raw = sys.stdin.buffer.read().decode("utf-8")
        req = json.loads(raw)
    except (json.JSONDecodeError, Exception) as e:
        log_util.log(f"[Download] 请求解析失败: {e}")
        _write_json({"ok": False, "error": f"请求解析失败: {e}"})
        return

    url = req.get("url", "").strip()
    file_name = req.get("file_name", "").strip()
    output_dir = req.get("output_dir", "").strip()
    language = req.get("language", "").strip()

    log_util.log(f"[Download] url={url}, file_name={file_name}, output_dir={output_dir}, language={language}")

    if not url:
        log_util.log("[Download] URL 为空")
        _write_json({"ok": False, "error": "下载 URL 不能为空"})
        return
    if not output_dir:
        log_util.log("[Download] 输出目录为空")
        _write_json({"ok": False, "error": "输出目录不能为空"})
        return
    if not file_name:
        file_name = url.split("/")[-1].split("?")[0]
        if not file_name:
            file_name = "subtitle.srt"

    try:
        log_util.log(f"[Download] 开始下载: {url}")
        result = _resolve_and_download(url, file_name, output_dir, language)
        log_util.log(f"[Download] 完成: {result}")
        _write_json(result)
    except Exception as e:
        tb = traceback.format_exc()
        log_util.log(f"[Download] 异常: {e}\n{tb}")
        _write_json({"ok": False, "error": str(e), "traceback": tb})


if __name__ == "__main__":
    main()