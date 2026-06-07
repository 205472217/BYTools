#pragma once

#include <QObject>
#include <QTimer>
#include <QProcess>

class FFmpegService : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegService(QObject *parent = nullptr);

    void startExtractAudio(const QString &ffmpegPath,
                           const QString &videoPath,
                           const QString &outputWav);

    void startBurnSubtitles(const QString &ffmpegPath,
                            const QString &videoPath,
                            const QString &srtPath,
                            const QString &outputPath,
                            int fontSize = 20,
                            const QString &fontColor = "#FFFFFF",
                            const QString &borderColor = "#000000",
                            int borderWidth = 2);

    void cancel();

    void setUseHardwareAccel(bool enable);

    /// 检测可用 GPU 加速类型
    /// @return 0=仅软件, 1=NVIDIA NVENC, 2=Intel QSV, 3=AMD AMF
    static int detectHardwareAccel(const QString &ffmpegPath);

    /// 检测可用 GPU 加速类型（人性化描述）
    static QString hardwareAccelName(const QString &ffmpegPath);

    static bool isFFmpegAvailable(const QString &ffmpegPath);
    static QString ffmpegVersion(const QString &ffmpegPath);
    static qint64 getVideoDuration(const QString &ffmpegPath, const QString &videoPath);

signals:
    void progress(double value);
    void finished(bool success, const QString &outputPath, const QString &error);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessTimeout();

private:
    QProcess *m_process;
    QTimer *m_timer;
    QString m_outputPath;
    bool m_isExtracting = false;
    bool m_useHardwareAccel = false;
    qint64 m_totalDuration = 0;

    // 根据 GPU 类型构建加速参数字符串列表
    QStringList buildGpuArgs(const QString &ffmpegPath,
                             const QString &videoPath,
                             const QString &styleFilter,
                             const QString &outputPath);

    // 检测输入视频编码格式
    static QString detectInputCodec(const QString &ffmpegPath,
                                    const QString &videoPath);

    // 缓存最近一次 stderr 输出（避免 onProcessReadyRead 独占消费，导致 onProcessFinished 读不到错误信息）
    QByteArray m_lastStderrBuffer;

    // 烧录参数缓存（GPU 失败时回退到软件重试）
    struct BurnSubtitleParams {
        QString ffmpegPath;
        QString videoPath;
        QString srtPath;
        QString outputPath;
        int fontSize = 20;
        QString fontColor = "#FFFFFF";
        QString borderColor = "#000000";
        int borderWidth = 2;
    };
    BurnSubtitleParams m_burnParams;
    bool m_burnFallbackTried = false;
};
