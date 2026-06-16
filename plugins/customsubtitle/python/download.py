#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
字幕文件下载脚本
从 stdin 读取 JSON 请求，下载字幕文件到指定目录，结果输出到 stdout。

请求格式 (stdin JSON):
  {"url": "https://...", "file_name": "xxx.srt", "output_dir": "/path/to/save"}

响应格式 (stdout JSON):
  {"ok": true, "file_path": "/path/to/save/xxx.srt", "size": 12345}
  或
  {"ok": false, "error": "错误描述"}
"""

import sys
import json
import os
import traceback


def download_file(url: str, output_dir: str, file_name: str) -> dict:
    """下载字幕文件"""
    try:
        from scrapling.fetchers import Fetcher
    except ImportError:
        return _download_file_fallback(url, output_dir, file_name)

    os.makedirs(output_dir, exist_ok=True)
    save_path = os.path.join(output_dir, file_name)

    try:
        resp = Fetcher.get(url, stealthy_headers=True, timeout=30)
        if resp.status != 200:
            return {"ok": False, "error": f"HTTP {resp.status}"}

        # 获取响应内容
        content = resp.body
        if not content:
            return {"ok": False, "error": "下载内容为空"}

        with open(save_path, "wb") as f:
            f.write(content)

        return {
            "ok": True,
            "file_path": save_path,
            "size": len(content),
        }
    except Exception as e:
        return {"ok": False, "error": str(e)}


def _download_file_fallback(url: str, output_dir: str, file_name: str) -> dict:
    """回退方案：使用 urllib 下载"""
    import urllib.request

    os.makedirs(output_dir, exist_ok=True)
    save_path = os.path.join(output_dir, file_name)

    try:
        req = urllib.request.Request(url, headers={
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                          "AppleWebKit/537.36 (KHTML, like Gecko) "
                          "Chrome/120.0.0.0 Safari/537.36"
        })
        with urllib.request.urlopen(req, timeout=30) as resp:
            content = resp.read()

        with open(save_path, "wb") as f:
            f.write(content)

        return {
            "ok": True,
            "file_path": save_path,
            "size": len(content),
        }
    except Exception as e:
        return {"ok": False, "error": f"urllib 下载失败: {e}"}


def main():
    try:
        raw = sys.stdin.read()
        req = json.loads(raw)
    except (json.JSONDecodeError, Exception) as e:
        print(json.dumps({"ok": False, "error": f"请求解析失败: {e}"}))
        sys.exit(1)

    url = req.get("url", "").strip()
    file_name = req.get("file_name", "").strip()
    output_dir = req.get("output_dir", "").strip()

    if not url:
        print(json.dumps({"ok": False, "error": "下载 URL 不能为空"}))
        sys.exit(1)
    if not output_dir:
        print(json.dumps({"ok": False, "error": "输出目录不能为空"}))
        sys.exit(1)
    if not file_name:
        # 从 URL 提取文件名
        file_name = url.split("/")[-1].split("?")[0]
        if not file_name:
            file_name = "subtitle.srt"

    try:
        result = download_file(url, output_dir, file_name)
        print(json.dumps(result, ensure_ascii=False))
    except Exception as e:
        tb = traceback.format_exc()
        print(json.dumps({"ok": False, "error": str(e), "traceback": tb}))
        sys.exit(1)


if __name__ == "__main__":
    main()