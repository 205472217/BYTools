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
import log_util


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
    here = os.path.dirname(os.path.abspath(__file__))
    log_util.init_log(here)

    try:
        raw = sys.stdin.read()
        req = json.loads(raw)
    except (json.JSONDecodeError, Exception) as e:
        log_util.log(f"请求解析失败: {e}")
        _write_json({"ok": False, "error": f"请求解析失败: {e}"})
        return

    site = req.get("site", "").strip()
    keyword = req.get("keyword", "").strip()
    language_filter = req.get("language_filter", "").strip()

    log_util.log(f"请求: site={site}, keyword={keyword}, lang={language_filter}")

    if not keyword:
        log_util.log("关键字为空")
        _write_json({"ok": False, "error": "关键字不能为空"})
        return

    max_results = req.get("max_results", 50)

    # 安全校验：只允许字母数字和下划线，防止路径穿越
    if not re.fullmatch(r"\w+", site):
        log_util.log(f"无效站点名: {site}")
        _write_json({"ok": False, "error": f"无效的站点名称: {site}"})
        return

    log_util.log(f"加载模块: {site}.search")
    try:
        mod = importlib.import_module(f"{site}.search")
    except ModuleNotFoundError:
        available = _list_available_sites()
        log_util.log(f"站点不支持: {site}，可选: {available}")
        _write_json({
            "ok": False,
            "error": f"不支持的站点: {site}，可选: {available}"
        })
        return

    log_util.log(f"开始搜索: site={site}, keyword={keyword}, max_results={max_results}")
    try:
        results = mod.search(keyword, language_filter, max_results)
        log_util.log(f"搜索完成: 结果数={len(results)}")
        _write_json({"ok": True, "results": results})
    except Exception as e:
        tb = traceback.format_exc()
        log_util.log(f"搜索异常: {e}\n{tb}")
        _write_json({"ok": False, "error": str(e), "traceback": tb})


if __name__ == "__main__":
    main()
