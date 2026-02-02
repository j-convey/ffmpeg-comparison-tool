#include "PredictTab.h"
#include "VideoUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>
#include <QFileInfo>

PredictTab::PredictTab(QWidget *parent) : QWidget(parent), predictProcess(nullptr) {
    setupUI();
}

PredictTab::~PredictTab() {
    if (predictProcess) {
        predictProcess->kill();
        predictProcess->waitForFinished();
        delete predictProcess;
    }
}

void PredictTab::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    // File Selection
    QGroupBox *predFileGroup = new QGroupBox("File Selection", this);
    QHBoxLayout *predFileLayout = new QHBoxLayout(predFileGroup);
    predFileEdit = new QLineEdit(this);
    predFileEdit->setPlaceholderText("Select video file...");
    QPushButton *predBrowseBtn = new QPushButton("Browse...", this);
    predFileLayout->addWidget(new QLabel("Input Video:", this));
    predFileLayout->addWidget(predFileEdit);
    predFileLayout->addWidget(predBrowseBtn);
    layout->addWidget(predFileGroup);

    // Settings
    QGroupBox *settingsGroup = new QGroupBox("ab-av1 Settings", this);
    QGridLayout *settingsLayout = new QGridLayout(settingsGroup);
    
    QLabel *encoderLabel = new QLabel("Encoder:", this);
    encoderLabel->setToolTip("Select the video codec. Software (lib*) is high quality but slow. Hardware (qsv/nvenc/amf) uses your GPU for speed.");
    settingsLayout->addWidget(encoderLabel, 0, 0);
    encoderCombo = new QComboBox(this);
    encoderCombo->addItems({
        "libsvtav1", "libx265", "libx264", 
        "av1_qsv", "hevc_qsv", "h264_qsv",
        "av1_nvenc", "hevc_nvenc", "h264_nvenc",
        "av1_amf", "hevc_amf", "h264_amf"
    });
    settingsLayout->addWidget(encoderCombo, 0, 1);

    QLabel *presetLabel = new QLabel("Preset:", this);
    presetLabel->setToolTip("Efficiency vs Speed.\n\n"
                            "Software: 'veryslow' to 'ultrafast'\n\n"
                            "Intel QSV:\n"
                            "1 = Quality (Slowest, best compression)\n"
                            "4 = Balanced\n"
                            "7 = Speed (Fastest)");
    settingsLayout->addWidget(presetLabel, 1, 0);
    presetEdit = new QLineEdit("8", this);
    presetEdit->setPlaceholderText("e.g., 8, medium, slow");
    settingsLayout->addWidget(presetEdit, 1, 1);

    QLabel *vmafLabel = new QLabel("Min VMAF:", this);
    vmafLabel->setToolTip("Target quality score (Netflix metric). 95 is standard for high quality. Higher values result in larger file sizes.");
    settingsLayout->addWidget(vmafLabel, 2, 0);
    vmafSpin = new QDoubleSpinBox(this);
    vmafSpin->setRange(0.0, 100.0);
    vmafSpin->setValue(95.0);
    settingsLayout->addWidget(vmafSpin, 2, 1);

    QLabel *samplesLabel = new QLabel("Samples:", this);
    samplesLabel->setToolTip("Number of video segments to analyze. More samples increase prediction accuracy but take longer to process.");
    settingsLayout->addWidget(samplesLabel, 3, 0);
    samplesSpin = new QSpinBox(this);
    samplesSpin->setRange(1, 50);
    samplesSpin->setValue(4);
    settingsLayout->addWidget(samplesSpin, 3, 1);
    layout->addWidget(settingsGroup);

    // Run & Cancel Buttons
    QHBoxLayout *actionLayout = new QHBoxLayout();
    predictRunBtn = new QPushButton("Run CRF Search", this);
    predictRunBtn->setMinimumHeight(40);
    
    predictCancelBtn = new QPushButton("Cancel", this);
    predictCancelBtn->setMinimumHeight(40);
    predictCancelBtn->setEnabled(false);

    actionLayout->addWidget(predictRunBtn);
    actionLayout->addWidget(predictCancelBtn);
    layout->addLayout(actionLayout);

    // Predict Progress Bar
    predictProgressBar = new QProgressBar(this);
    predictProgressBar->setVisible(false);
    predictProgressBar->setTextVisible(true);
    layout->addWidget(predictProgressBar);

    // Predict Results Group
    predResultsGroup = new QGroupBox("Prediction Results", this);
    predResultsGroup->setVisible(false);
    QGridLayout *predResLayout = new QGridLayout(predResultsGroup);
    
    predResultCRFLabel = new QLabel("--", this);
    predResultVMAFLabel = new QLabel("--", this);
    predResultSizeLabel = new QLabel("--", this);
    predResultTimeLabel = new QLabel("--", this);
    
    QString labelStyle = "QLabel { font-size: 11pt; font-weight: bold; }";
    predResultCRFLabel->setStyleSheet(labelStyle);
    predResultVMAFLabel->setStyleSheet(labelStyle);
    predResultSizeLabel->setStyleSheet(labelStyle);
    predResultTimeLabel->setStyleSheet(labelStyle);

    predResLayout->addWidget(new QLabel("Best CRF:", this), 0, 0);
    predResLayout->addWidget(predResultCRFLabel, 0, 1);
    predResLayout->addWidget(new QLabel("Predicted VMAF:", this), 0, 2);
    predResLayout->addWidget(predResultVMAFLabel, 0, 3);
    predResLayout->addWidget(new QLabel("Predicted Size:", this), 1, 0);
    predResLayout->addWidget(predResultSizeLabel, 1, 1);
    predResLayout->addWidget(new QLabel("Est. Time:", this), 1, 2);
    predResLayout->addWidget(predResultTimeLabel, 1, 3);
    layout->addWidget(predResultsGroup);

    // Output Log
    predictOutput = new QTextEdit(this);
    predictOutput->setReadOnly(true);
    predictOutput->setFont(QFont("Courier New", 9));
    layout->addWidget(predictOutput);

    // Connections
    connect(predBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Video", "", "Video Files (*.mp4 *.mkv *.mov *.webm *.avi)");
        if (!fileName.isEmpty()) predFileEdit->setText(fileName);
    });

    connect(predictCancelBtn, &QPushButton::clicked, this, [this]() {
        if (predictProcess && predictProcess->state() != QProcess::NotRunning) {
            predictOutput->append("\nCancelling process...");
            predictProcess->kill();
        }
    });

    connect(predictRunBtn, &QPushButton::clicked, this, [this]() {
        QString inputFile = predFileEdit->text();
        if (inputFile.isEmpty() || !QFileInfo::exists(inputFile) || !VideoUtils::isValidVideoFile(inputFile)) {
            QMessageBox::warning(this, "Error", "Please select a valid input file.");
            return;
        }

        predictRunBtn->setEnabled(false);
        predictRunBtn->setText("Running ab-av1...");
        predictOutput->clear();
        predictOutput->append("Starting CRF search...");
        predictProgressBar->setValue(0);
        predictProgressBar->setVisible(true);
        predResultsGroup->setVisible(false);
        predictCancelBtn->setEnabled(true);

        if (predictProcess) {
            delete predictProcess;
            predictProcess = nullptr;
        }
        predictProcess = new QProcess(this);
        QProcess *proc = predictProcess;
        
        QStringList args;
        args << "crf-search" << "-i" << inputFile
             << "--encoder" << encoderCombo->currentText()
             << "--preset" << presetEdit->text()
             << "--min-vmaf" << QString::number(vmafSpin->value())
             << "--samples" << QString::number(samplesSpin->value());

        predictOutput->append("Executing: ab-av1 " + args.join(" "));
        predictOutput->append("--------------------------------------------------");

        // Shared output handler
        auto handleOutput = [this, proc](bool isError) {
            QByteArray data = isError ? proc->readAllStandardError() : proc->readAllStandardOutput();
            QString output = QString::fromLocal8Bit(data).trimmed();
            if (output.isEmpty()) return;
            
            predictOutput->append(output);
            predictOutput->verticalScrollBar()->setValue(predictOutput->verticalScrollBar()->maximum());
            
            // Parse Progress: "00:00:01 crf 30 4/4 #..."
            static QRegularExpression progressRegex("\\s(\\d+)/(\\d+)\\s");
            QRegularExpressionMatch progMatch = progressRegex.match(output);
            if (progMatch.hasMatch()) {
                int current = progMatch.captured(1).toInt();
                int total = progMatch.captured(2).toInt();
                if (total > 0) {
                    int percent = (current * 100) / total;
                    predictProgressBar->setValue(percent);
                    predictProgressBar->setFormat(QString("Processing sample %1/%2 (%3%)").arg(current).arg(total).arg(percent));
                }
            }

            // Parse Result: "crf 30 VMAF 95.74 predicted video stream size 4.33 GiB (65%) taking 25 minutes"
            static QRegularExpression resultRegex("crf (\\d+) VMAF ([\\d.]+) predicted video stream size (.*?) taking (.*)");
            QRegularExpressionMatch resMatch = resultRegex.match(output);
            if (resMatch.hasMatch()) {
                QString crf = resMatch.captured(1);
                double vmaf = resMatch.captured(2).toDouble();
                QString sizeStr = resMatch.captured(3);
                QString time = resMatch.captured(4).trimmed();
                
                predResultCRFLabel->setText(crf);
                predResultVMAFLabel->setText(QString::number(vmaf, 'f', 2));
                predResultSizeLabel->setText(sizeStr);
                predResultTimeLabel->setText(time);
                
                // Color code VMAF
                QString vmafStyle = "QLabel { font-size: 12pt; font-weight: bold; padding: 5px; border-radius: 3px; ";
                if (vmaf >= 95.0) vmafStyle += "background-color: #4caf50; color: white; }"; // Excellent
                else if (vmaf >= 85.0) vmafStyle += "background-color: #8bc34a; color: white; }"; // Very Good
                else if (vmaf >= 75.0) vmafStyle += "background-color: #ffeb3b; color: black; }"; // Good
                else if (vmaf >= 60.0) vmafStyle += "background-color: #ff9800; color: white; }"; // Fair
                else vmafStyle += "background-color: #f44336; color: white; }"; // Poor
                
                predResultVMAFLabel->setStyleSheet(vmafStyle);
                // predResultCRFLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 5px; background-color: #4caf50; border-radius: 3px; }");
                predResultsGroup->setVisible(true);
            }
        };

        connect(proc, &QProcess::readyReadStandardOutput, this, [handleOutput]() { handleOutput(false); });
        connect(proc, &QProcess::readyReadStandardError, this, [handleOutput]() { handleOutput(true); });

        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
                this, [=](int exitCode, QProcess::ExitStatus status) {
            predictRunBtn->setEnabled(true);
            predictRunBtn->setText("Run CRF Search");
            predictCancelBtn->setEnabled(false);
            predictProgressBar->setVisible(false);
            if (status == QProcess::NormalExit && exitCode == 0) {
                predictOutput->append("\nSUCCESS: CRF search completed.");
                
                // Add to history
                QString details = QString("%1 (%2, %3)")
                    .arg(QFileInfo(inputFile).fileName())
                    .arg(encoderCombo->currentText())
                    .arg(presetEdit->text());
                
                QString result = QString("CRF: %1 | VMAF: %2 | Size: %3 | Time: %4")
                    .arg(predResultCRFLabel->text())
                    .arg(predResultVMAFLabel->text())
                    .arg(predResultSizeLabel->text())
                    .arg(predResultTimeLabel->text());
                    
                emit predictionCompleted("Prediction", details, result);
            } else {
                predictOutput->append("\nFAILED: Process exited with code " + QString::number(exitCode));
            }
            predictProcess->deleteLater();
            predictProcess = nullptr;
        });

        proc->start("ab-av1", args);
        if (!proc->waitForStarted()) {
            predictOutput->append("Error: Failed to start ab-av1. Ensure it is in your PATH.");
            predictRunBtn->setEnabled(true);
            predictRunBtn->setText("Run CRF Search");
            proc->deleteLater();
        }
    });
}