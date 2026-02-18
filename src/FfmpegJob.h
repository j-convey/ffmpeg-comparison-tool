#ifndef FFMPEGJOB_H
#define FFMPEGJOB_H

#include <QObject>
#include <QProcess>
#include <QString>

struct SsimResult {
    double y = 0, u = 0, v = 0, all = 0;
    QString yDb, uDb, vDb, allDb;
};

struct PsnrResult {
    QString yDb, uDb, vDb, avgDb;
};

// Encapsulates running an ffmpeg SSIM/PSNR/VMAF comparison as a background process.
// The tab connects to the signals to drive UI updates; it never touches QProcess directly.
class FfmpegJob : public QObject {
    Q_OBJECT

public:
    explicit FfmpegJob(QObject *parent = nullptr);
    ~FfmpegJob();

    // startTime / duration: pass a non-empty HH:MM:SS string to apply -ss / -t flags.
    // Pass empty strings to omit them.
    void start(const QString& originalFile, const QString& comparisonFile,
               const QString& startTime = QString(), const QString& duration = QString());
    void cancel();
    bool isRunning() const;

signals:
    // Raw text line from the process
    void logLine(const QString& line);
    // Progress: currentTime and totalDuration in seconds
    void progressUpdated(double currentTime, double totalDuration);
    // Parsed quality metric results
    void ssimResult(const SsimResult& result);
    void psnrResult(const PsnrResult& result);
    void vmafResult(double score);
    // Process exited; success == (NormalExit && exitCode == 0)
    void finished(bool success, int exitCode);

private:
    void parseStderr(const QString& text);
    static double hmsToSeconds(const QString& h, const QString& m, const QString& s);

    QProcess *m_process = nullptr;
    double m_totalDuration = 0.0;
    double m_currentTime   = 0.0;
};

#endif // FFMPEGJOB_H
