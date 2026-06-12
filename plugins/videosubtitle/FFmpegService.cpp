#include "FFmpegService.h"
#include "Logger.h"

FFmpegService::FFmpegService(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_runner(new FfmpegRunner(this))
{
    m_runner->setLogger(m_logger);
    connect(m_runner, &FfmpegRunner::progress,
            this, &FFmpegService::progress);
    connect(m_runner, &FfmpegRunner::finished,
            this, &FFmpegService::onRunnerFinished);
}

void FFmpegService::setUseHardwareAccel(bool enable)
{
    m_checkUseGpu = enable;
}

QString FFmpegService::hardwareAccelName(const QString &ffmpegPath)
{
    return gpuVendorName(detectGpuVendor(ffmpegPath));
}

void FFmpegService::cancel()
{
    m_runner->cancel();
}

// ── Public API ────────────────────────────────────────────

void FFmpegService::startExtractAudio(const QString &ffmpegPath,
                                       const QString &videoPath,
                                       const QString &outputWav)
{
    m_outputPath = outputWav;
    m_isExtracting = true;

    FfmpegRunner::ExtractAudioConfig cfg;
    cfg.ffmpegPath    = ffmpegPath;
    cfg.videoPath     = videoPath;
    cfg.outputWavPath = outputWav;

    m_runner->extractAudio(cfg);
    m_logger->info("FFmpeg 音频提取已启动");
}

void FFmpegService::startBurnSubtitles(const QString &ffmpegPath,
                                        const QString &videoPath,
                                        const QString &srtPath,
                                        const QString &outputPath,
                                        int fontSize,
                                        const QString &fontColor,
                                        const QString &borderColor,
                                        int borderWidth)
{
    m_outputPath = outputPath;
    m_isExtracting = false;

    FfmpegRunner::BurnConfig cfg;
    cfg.ffmpegPath   = ffmpegPath;
    cfg.videoPath    = videoPath;
    cfg.subtitlePath = srtPath;
    cfg.outputPath   = outputPath;
    cfg.useGpu       = m_checkUseGpu;
    cfg.fontName     = "Microsoft YaHei";
    cfg.fontSize     = fontSize;
    cfg.fontColor    = fontColor;
    cfg.borderColor  = borderColor;
    cfg.borderWidth  = borderWidth;

    m_runner->burnSubtitles(cfg);

    if (m_checkUseGpu) {
        m_logger->info("使用 GPU 硬件加速烧录字幕");
    }
    m_logger->info("FFmpeg 烧录已启动");
}

// ── Runner slots ──────────────────────────────────────────

void FFmpegService::onRunnerFinished(bool success, const QString &outputPath, const QString &error)
{
    Q_UNUSED(outputPath)
    emit finished(success, m_outputPath, error);
}
