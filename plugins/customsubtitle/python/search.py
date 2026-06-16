#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
字幕搜索入口脚本
从 stdin 读取 JSON 请求，按 site 字段动态加载 <site>/search.py 模块。
每个站点一个独立文件夹，修改某站点搜索逻辑不影响其他站点。

请求格式 (stdin JSON):
  {"site": "subtitlecat", "keyword": "matrix"}

响应格式 (stdout JSON):
  {"ok": true, "results": [
      {"site": "SubtitleCat", "language": "English",
       "file_name": "xxx.srt", "download_url": "https://..."}
  ]}
  或
  {"ok": false, "error": "错误描述"}
"""

import sys
import json
import os
import re
import importlib
import traceback


def _list_available_sites() -> list:
    """扫描 python/ 目录，返回含 search.py 的文件夹列表"""
    here = os.path.dirname(os.path.abspath(__file__))
    sites = []
    for name in os.listdir(here):
        dirpath = os.path.join(here, name)
        if os.path.isdir(dirpath) and os.path.isfile(os.path.join(dirpath, "search.py")):
            sites.append(name)
    return sorted(sites)


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

    site = req.get("site", "").strip()
    keyword = req.get("keyword", "").strip()
    language_filter = req.get("language_filter", "").strip()

    if not keyword:
        _write_json({"ok": False, "error": "关键字不能为空"})
        return

    # 安全校验：只允许字母数字和下划线，防止路径穿越
    if not re.fullmatch(r"\w+", site):
        _write_json({"ok": False, "error": f"无效的站点名称: {site}"})
        return

    try:
        mod = importlib.import_module(f"{site}.search")
    except ModuleNotFoundError:
        available = _list_available_sites()
        _write_json({
            "ok": False,
            "error": f"不支持的站点: {site}，可选: {available}"
        })
        return

    try:
        results = mod.search(keyword, language_filter)
        _write_json({"ok": True, "results": results})
    except Exception as e:
        tb = traceback.format_exc()
        _write_json({"ok": False, "error": str(e), "traceback": tb})


if __name__ == "__main__":
    main()
