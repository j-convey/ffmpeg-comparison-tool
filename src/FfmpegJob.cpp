#include "FfmpegJob.h"
#include <QRegularExpression>

FfmpegJob::FfmpegJob(QObject *parent) : QObject(parent) {}

FfmpegJob::~FfmpegJob() {
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished();
        delete m_process;
    }
}

bool FfmpegJob::isRunning() const {
    return m_process && m_process->state() != QProcess::NotRunning;
}

void FfmpegJob::cancel() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
}

// static helper
double FfmpegJob::hmsToSeconds(const QString& h, const QString& m, const QString& s) {
    return h.toInt() * 3600.0 + m.toInt() * 60.0 + s.toInt();
}

void FfmpegJob::start(const QString& originalFile, const QString& comparisonFile,
                      const QString& startTime, const QString& duration) {
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_totalDuration = 0.0;
    m_currentTime   = 0.0;

    // If duration was provided explicitly, pre-seed totalDuration so progress works immediately.
    if (!duration.isEmpty()) {
        QStringList parts = duration.split(":");
        if (parts.size() == 3)
            m_totalDuration = hmsToSeconds(parts[0], parts[1], parts[2]);
    }

    // Build ffmpeg argument list
    QStringList arguments;

    if (!startTime.isEmpty()) arguments << "-ss" << startTime;
    if (!duration.isEmpty())  arguments << "-t"  << duration;
    arguments << "-i" << originalFile;

    if (!startTime.isEmpty()) arguments << "-ss" << startTime;
    if (!duration.isEmpty())  arguments << "-t"  << duration;
    arguments << "-i" << comparisonFile;

    QString filterComplex =
        "[0:v]split=3[ref1][ref2][ref3];"
        "[1:v]split=3[main1][main2][main3];"
        "[main1][ref1]ssim[stats_ssim];"
        "[main2][ref2]psnr[stats_psnr];"
        "[main3][ref3]libvmaf";

    arguments << "-filter_complex" << filterComplex
              << "-map" << "[stats_ssim]"
              << "-map" << "[stats_psnr]"
              << "-f"   << "null" << "-";

    emit logLine("Running command:");
    emit logLine("ffmpeg " + arguments.join(" "));
    emit logLine("\n" + QString("-").repeated(80) + "\n");

    m_process = new QProcess(this);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        emit logLine(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        parseStderr(QString::fromLocal8Bit(m_process->readAllStandardError()));
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        bool success = (status == QProcess::NormalExit && exitCode == 0);
        emit finished(success, exitCode);
        m_process->deleteLater();
        m_process = nullptr;
    });

    m_process->start("ffmpeg", arguments);
    if (!m_process->waitForStarted()) {
        emit finished(false, -1);
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void FfmpegJob::parseStderr(const QString& text) {
    emit logLine(text);

    // --- Duration (ffmpeg header) ---
    if (m_totalDuration == 0.0) {
        static QRegularExpression durationRx(R"(Duration: (\d{2}):(\d{2}):(\d{2})\.)");
        QRegularExpressionMatch m = durationRx.match(text);
        if (m.hasMatch())
            m_totalDuration = hmsToSeconds(m.captured(1), m.captured(2), m.captured(3));
    }

    // --- Progress: "time=HH:MM:SS.xx" ---
    static QRegularExpression progressRx(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    QRegularExpressionMatch progMatch = progressRx.match(text);
    if (progMatch.hasMatch() && m_totalDuration > 0.0) {
        m_currentTime = hmsToSeconds(progMatch.captured(1), progMatch.captured(2), progMatch.captured(3));
        emit progressUpdated(m_currentTime, m_totalDuration);
    }

    // --- SSIM: "SSIM Y:X.XXXX (db) U:... V:... All:X.XXXX (db)" ---
    static QRegularExpression ssimRx(
        R"(SSIM Y:([\d.]+) \(([\d.]+|inf)\) U:([\d.]+) \(([\d.]+|inf)\) V:([\d.]+) \(([\d.]+|inf)\) All:([\d.]+) \(([\d.]+|inf)\))");
    QRegularExpressionMatch ssimMatch = ssimRx.match(text);
    if (ssimMatch.hasMatch()) {
        SsimResult r;
        r.y    = ssimMatch.captured(1).toDouble(); r.yDb  = ssimMatch.captured(2);
        r.u    = ssimMatch.captured(3).toDouble(); r.uDb  = ssimMatch.captured(4);
        r.v    = ssimMatch.captured(5).toDouble(); r.vDb  = ssimMatch.captured(6);
        r.all  = ssimMatch.captured(7).toDouble(); r.allDb = ssimMatch.captured(8);
        emit ssimResult(r);
    }

    // --- PSNR: "PSNR ... y:X u:X v:X average:X ..." ---
    static QRegularExpression psnrRx(
        R"(PSNR.*?y:([\d.]+|inf).*?u:([\d.]+|inf).*?v:([\d.]+|inf).*?average:([\d.]+|inf))");
    QRegularExpressionMatch psnrMatch = psnrRx.match(text);
    if (psnrMatch.hasMatch()) {
        PsnrResult r;
        r.yDb = psnrMatch.captured(1); r.uDb  = psnrMatch.captured(2);
        r.vDb = psnrMatch.captured(3); r.avgDb = psnrMatch.captured(4);
        emit psnrResult(r);
    }

    // --- VMAF: "VMAF score: X.XX" (or "VMAF score = X.XX") ---
    static QRegularExpression vmafRx(R"(VMAF score[:\s=]+([\d.]+))");
    QRegularExpressionMatch vmafMatch = vmafRx.match(text);
    if (vmafMatch.hasMatch())
        emit vmafResult(vmafMatch.captured(1).toDouble());
}
