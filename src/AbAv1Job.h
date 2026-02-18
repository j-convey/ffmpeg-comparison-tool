#ifndef ABAV1JOB_H
#define ABAV1JOB_H

#include <QObject>
#include <QProcess>
#include <QString>

// Encapsulates running an ab-av1 crf-search as a background process.
// The tab connects to the signals to drive UI updates; it never touches QProcess directly.
class AbAv1Job : public QObject {
    Q_OBJECT

public:
    explicit AbAv1Job(QObject *parent = nullptr);
    ~AbAv1Job();

    void start(const QString& inputFile, const QString& encoder, const QString& preset,
               double minVmaf, int samples);
    void cancel();
    bool isRunning() const;

signals:
    // Raw text line from the process (stdout or stderr)
    void logLine(const QString& line);
    // Parsed "x/total" progress from ab-av1 output
    void progressUpdated(int current, int total);
    // Parsed final prediction result
    void resultReady(const QString& crf, double vmaf, const QString& size, const QString& time);
    // Process exited; success == (NormalExit && exitCode == 0)
    void finished(bool success, int exitCode);

private:
    void handleOutput(const QByteArray& data);

    QProcess *m_process = nullptr;
};

#endif // ABAV1JOB_H
