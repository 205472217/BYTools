#pragma once

#include <QString>
#include <QStringList>

class PluginLogger;

// ── GPU 厂商枚举 ─────────────────────────────────────────
// 统一自 FFmpegMergeService (enum class) 和 FFmpegService (int)

enum class GpuVendor {
    None,
    CUDA,   // NVIDIA NVENC
    AMD,    // AMD AMF
    Intel   // Intel QSV
};

// ── GPU 检测 ─────────────────────────────────────────────

QString gpuVendorName(GpuVendor vendor);

/// 自动检测可用的 GPU 编码器，探测顺序：NVIDIA → AMD → Intel
/// 返回 GpuVendor::None 表示无可用的硬件编码器
GpuVendor detectGpuVendor(const QString &ffmpegPath);

// ── 视频编码检测 ─────────────────────────────────────────

/// 从 ffmpeg -i 输出中检测输入视频的编码格式
/// 返回 "h264", "hevc", "av1"，默认 "h264"
QString detectInputCodec(const QString &ffmpegPath, const QString &videoPath);

// ── 字幕滤镜构建 ─────────────────────────────────────────
/// 构建 subtitles 滤镜参数字符串（单引号包裹路径，兼容 FFmpeg 4.0）
/// 颜色参数为 HTML 格式 #RRGGBB，内部自动转为 ASS &H00BBGGRR 格式
QString buildSubtitleFilter(const QString &subtitlePath,
                            const QString &fontName = "Microsoft YaHei",
                            int fontSize = 18,
                            const QString &fontColor = "#FFFFFF",
                            const QString &borderColor = "#000000",
                            int borderWidth = 1,
                            int shadow = 0);

// ── FFmpeg 工具函数 ───────────────────────────────────────

/// 检查 ffmpeg 是否可用（启动测试 + 版本检查，含详细错误日志）
/// @param logger  可选，传入则输出日志到对应 logger
bool isFFmpegAvailable(const QString &ffmpegPath, PluginLogger *logger = nullptr);

/// 获取 ffmpeg 版本字符串（如 "7.0.2"），不可用时返回空
QString ffmpegVersion(const QString &ffmpegPath);

/// 获取视频时长（毫秒），失败返回 0
qint64 getVideoDuration(const QString &ffmpegPath, const QString &videoPath);

/// 获取视频流的编码码率（bps），基于 ffmpeg -i 输出解析
/// 优先读取视频流码率（如 "1048 kb/s"），回退到总码率估算音频后取值，失败返回 0
/// 相比 getVideoBitrate，这个值不包含音频码率，更适合传给 -b:v
qint64 getVideoStreamBitrate(const QString &ffmpegPath, const QString &videoPath);

/// 获取视频帧率（fps），基于 ffmpeg -i 输出解析，失败返回 0
/// 解析 Stream #0:0 ... 末尾的 fps 字段（如 "23.98 fps", "59.97 fps", "30 fps"）
qint64 getVideoFps(const QString &ffmpegPath, const QString &videoPath);

/// 从 ffmpeg stderr 中提取人类可读的错误信息
/// 清理内存地址、进度行、boilerplate 噪声
QString extractFfmpegError(const QString &rawStderr);

// ── GPU 加速参数构建 ─────────────────────────────────────
/// 构建 GPU 加速的 ffmpeg 命令行参数
/// vendor: 由 detectGpuVendor 返回
/// subtitleFilter: 由 buildSubtitleFilter 返回的完整 subtitles 滤镜字符串
/// inputCodec: 由 detectInputCodec 返回的编码类型
/// 返回空列表表示不支持（vendor == None）
QStringList buildGpuAccelArgs(GpuVendor vendor,
                              const QString &videoPath,
                              const QString &subtitleFilter,
                              const QString &outputPath,
                              const QString &inputCodec,
                              qint64 bitrate = 0,
                              qint64 fps = 0);
