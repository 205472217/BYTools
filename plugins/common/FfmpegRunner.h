#pragma once

#include "ProcessRunner.h"
#include "FfmpegUtils.h"

class PluginLogger;

/// 黑盒封装 ffmpeg 烧录字幕 + 提取音频操作。
/// 调用者只需填配置、连信号，不关心 GPU 检测/参数构建/进度解析/回退等内部细节。
class FfmpegRunner : public ProcessRunner
{
    Q_OBJECT
public:
    // ── 配置结构体 ──────────────────────────────────────

    struct BurnConfig {
        QString ffmpegPath;
        QString videoPath;
        QString subtitlePath;
        QString outputPath;
        bool useGpu = false;
        GpuVendor gpuVendor = GpuVendor::None;  // 可选，调用者预先检测好的
        bool useFragMp4 = false;
        QString fontName  = "Microsoft YaHei";
        int     fontSize   = 20;
        QString fontColor  = "#FFFFFF";
        QString borderColor = "#000000";
        int     borderWidth = 2;
    };

    struct ExtractAudioConfig {
        QString ffmpegPath;
        QString videoPath;
        QString outputWavPath;
        int sampleRate = 16000;
        int channels = 1;
    };

    explicit FfmpegRunner(QObject *parent = nullptr);

    void burnSubtitles(const BurnConfig &config);
    void extractAudio(const ExtractAudioConfig &config);
    void cancel();

    /// 设置日志记录器（可选，不设置则跳过日志输出）
    void setLogger(PluginLogger *logger) { ProcessRunner::setLogger(logger); }

signals:
    void progress(double value);   // 0.0 ~ 1.0（基于视频时长）
    void finished(bool success, const QString &outputPath, const QString &error);

protected:
    void onStderrData(const QByteArray &data) override;
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) override;
    void onProcessTimeout() override;

private:
    void buildBurnArgs(const BurnConfig &config, QStringList &args);

    enum class Operation { None, Burn, ExtractAudio };
    Operation m_operation = Operation::None;

    // Burn state
    BurnConfig m_burnConfig;
    bool m_burnFallbackTried = false;
    qint64 m_totalDuration = 0;

    // ExtractAudio state
    QString m_audioOutputPath;
};
