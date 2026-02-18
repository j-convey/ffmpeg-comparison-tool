#include "AbAv1Job.h"
#include <QRegularExpression>

AbAv1Job::AbAv1Job(QObject *parent) : QObject(parent) {}

AbAv1Job::~AbAv1Job() {
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished();
        delete m_process;
    }
}

bool AbAv1Job::isRunning() const {
    return m_process && m_process->state() != QProcess::NotRunning;
}

void AbAv1Job::cancel() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        emit logLine("\nCancelling process...");
        m_process->kill();
    }
}

void AbAv1Job::start(const QString& inputFile, const QString& encoder, const QString& preset,
                     double minVmaf, int samples) {
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_process = new QProcess(this);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        handleOutput(m_process->readAllStandardOutput());
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        handleOutput(m_process->readAllStandardError());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        bool success = (status == QProcess::NormalExit && exitCode == 0);
        emit finished(success, exitCode);
        m_process->deleteLater();
        m_process = nullptr;
    });

    QStringList args;
    args << "crf-search" << "-i" << inputFile
         << "--encoder" << encoder
         << "--preset" << preset
         << "--min-vmaf" << QString::number(minVmaf)
         << "--samples" << QString::number(samples);

    emit logLine("Executing: ab-av1 " + args.join(" "));
    emit logLine("--------------------------------------------------");

    m_process->start("ab-av1", args);
    if (!m_process->waitForStarted()) {
        emit logLine("Error: Failed to start ab-av1. Ensure it is in your PATH.");
        emit finished(false, -1);
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void AbAv1Job::handleOutput(const QByteArray& data) {
    QString output = QString::fromLocal8Bit(data).trimmed();
    if (output.isEmpty()) return;

    emit logLine(output);

    // Parse progress: some text "N/total" some text
    static QRegularExpression progressRegex(R"(\s(\d+)/(\d+)\s)");
    QRegularExpressionMatch progMatch = progressRegex.match(output);
    if (progMatch.hasMatch()) {
        int current = progMatch.captured(1).toInt();
        int total   = progMatch.captured(2).toInt();
        if (total > 0) emit progressUpdated(current, total);
    }

    // Parse final result: "crf N VMAF X.XX predicted video stream size S taking T"
    static QRegularExpression resultRegex(R"(crf (\d+) VMAF ([\d.]+) predicted video stream size (.*?) taking (.*))");
    QRegularExpressionMatch resMatch = resultRegex.match(output);
    if (resMatch.hasMatch()) {
        emit resultReady(
            resMatch.captured(1),
            resMatch.captured(2).toDouble(),
            resMatch.captured(3),
            resMatch.captured(4).trimmed()
        );
    }
}
