import os
import sys
import json
from datetime import datetime

_LOG_FILE = None


def init_log(scripts_dir=None):
    global _LOG_FILE
    if scripts_dir is None:
        scripts_dir = os.path.dirname(os.path.abspath(__file__))
    _LOG_FILE = os.path.join(scripts_dir, "search_py.log")
    log("=== 搜索进程启动 ===")


def log(*args):
    if _LOG_FILE is None:
        init_log()
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
    msg = f"[{timestamp}] {' '.join(str(a) for a in args)}"
    try:
        with open(_LOG_FILE, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception:
        pass


def write_progress(current, total, message=""):
    """写进度 JSON 到 stdout，C++ 侧实时读取"""
    obj = {"progress": current, "total": total, "message": message}
    raw = json.dumps(obj, ensure_ascii=True)
    sys.stdout.buffer.write(raw.encode("utf-8"))
    sys.stdout.buffer.write(b"\n")
    sys.stdout.buffer.flush()
