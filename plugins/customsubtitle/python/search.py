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


def main():
    try:
        raw = sys.stdin.read()
        req = json.loads(raw)
    except (json.JSONDecodeError, Exception) as e:
        print(json.dumps({"ok": False, "error": f"请求解析失败: {e}"}))
        sys.exit(1)

    site = req.get("site", "").strip()
    keyword = req.get("keyword", "").strip()

    if not keyword:
        print(json.dumps({"ok": False, "error": "关键字不能为空"}))
        sys.exit(1)

    # 安全校验：只允许字母数字和下划线，防止路径穿越
    if not re.fullmatch(r"\w+", site):
        print(json.dumps({"ok": False, "error": f"无效的站点名称: {site}"}))
        sys.exit(1)

    try:
        mod = importlib.import_module(f"{site}.search")
    except ModuleNotFoundError:
        available = _list_available_sites()
        print(json.dumps({
            "ok": False,
            "error": f"不支持的站点: {site}，可选: {available}"
        }))
        sys.exit(1)

    try:
        results = mod.search(keyword)
        print(json.dumps({"ok": True, "results": results}, ensure_ascii=False))
    except Exception as e:
        tb = traceback.format_exc()
        print(json.dumps({"ok": False, "error": str(e), "traceback": tb}))
        sys.exit(1)


if __name__ == "__main__":
    main()
