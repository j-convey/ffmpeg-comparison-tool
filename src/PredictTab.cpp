#include "PredictTab.h"
#include "VideoUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

PredictTab::PredictTab(QWidget *parent) : QWidget(parent) {
    predictJob = new AbAv1Job(this);
    setupUI();

    // Log lines → output widget
    connect(predictJob, &AbAv1Job::logLine, this, [this](const QString& line) {
        predictOutput->append(line);
        predictOutput->verticalScrollBar()->setValue(predictOutput->verticalScrollBar()->maximum());
    });

    // Progress updates → progress bar
    connect(predictJob, &AbAv1Job::progressUpdated, this, [this](int current, int total) {
        int percent = (current * 100) / total;
        predictProgressBar->setValue(percent);
        predictProgressBar->setFormat(
            QString("Processing sample %1/%2 (%3%)").arg(current).arg(total).arg(percent));
    });

    // Prediction result → result labels
    connect(predictJob, &AbAv1Job::resultReady, this,
            [this](const QString& crf, double vmaf, const QString& size, const QString& time) {
        predResultCRFLabel->setText(crf);
        predResultVMAFLabel->setText(QString::number(vmaf, 'f', 2));
        predResultSizeLabel->setText(size);
        predResultTimeLabel->setText(time);

        QString vmafStyle = "QLabel { font-size: 12pt; font-weight: bold; padding: 5px; border-radius: 3px; ";
        if      (vmaf >= 95.0) vmafStyle += "background-color: #4caf50; color: white; }";
        else if (vmaf >= 85.0) vmafStyle += "background-color: #8bc34a; color: white; }";
        else if (vmaf >= 75.0) vmafStyle += "background-color: #ffeb3b; color: black; }";
        else if (vmaf >= 60.0) vmafStyle += "background-color: #ff9800; color: white; }";
        else                   vmafStyle += "background-color: #f44336; color: white; }";

        predResultVMAFLabel->setStyleSheet(vmafStyle);
        predResultCRFLabel->setStyleSheet(
            "QLabel { font-size: 12pt; font-weight: bold; padding: 5px; "
            "background-color: #e3f2fd; border-radius: 3px; }");
        predResultsGroup->setVisible(true);
    });

    // Job finished → restore UI and emit history signal
    connect(predictJob, &AbAv1Job::finished, this, [this](bool success, int exitCode) {
        predictRunBtn->setEnabled(true);
        predictRunBtn->setText("Run CRF Search");
        predictCancelBtn->setEnabled(false);
        predictProgressBar->setVisible(false);
        if (success) {
            predictOutput->append("\nSUCCESS: CRF search completed.");
            QString result = QString("CRF: %1 | VMAF: %2 | Size: %3 | Time: %4")
                .arg(predResultCRFLabel->text())
                .arg(predResultVMAFLabel->text())
                .arg(predResultSizeLabel->text())
                .arg(predResultTimeLabel->text());
            emit predictionCompleted("Prediction", m_pendingRunDetails, result);
        } else {
            predictOutput->append("\nFAILED: Process exited with code " + QString::number(exitCode));
        }
    });
}

void PredictTab::updatePresetOptions(const QString &encoder) {
    presetCombo->clear();

    // Helper lambda: adds an item with display text and the raw preset value as UserRole data
    auto add = [this](const QString &display, const QString &value) {
        presetCombo->addItem(display, QVariant(value));
    };

    if (encoder == "av1_qsv" || encoder == "hevc_qsv" || encoder == "h264_qsv") {
        // Intel QSV: numbers 1–7 matching HandBrake's quality/balanced/speed labels
        add("1 (quality)",   "1");
        add("2",             "2");
        add("3",             "3");
        add("4 (balanced)",  "4");
        add("5",             "5");
        add("6",             "6");
        add("7 (speed)",     "7");
        presetCombo->setCurrentIndex(3); // default: balanced
        presetCombo->setToolTip("Intel QSV preset (1–7).\n1 = quality (slowest, best compression)\n4 = balanced\n7 = speed (fastest)");

    } else if (encoder == "av1_nvenc" || encoder == "hevc_nvenc" || encoder == "h264_nvenc") {
        // NVIDIA NVENC: p1 (fastest) – p7 (slowest/best quality), matching HandBrake labels
        add("p7 (quality)",  "p7");
        add("p6",            "p6");
        add("p5",            "p5");
        add("p4 (balanced)", "p4");
        add("p3",            "p3");
        add("p2",            "p2");
        add("p1 (speed)",    "p1");
        presetCombo->setCurrentIndex(3); // default: balanced
        presetCombo->setToolTip("NVIDIA NVENC preset (p1–p7).\np7 = quality (slowest)\np4 = balanced\np1 = speed (fastest)");

    } else if (encoder == "av1_amf" || encoder == "hevc_amf" || encoder == "h264_amf") {
        // AMD AMF: named presets matching HandBrake
        add("quality",   "quality");
        add("balanced",  "balanced");
        add("speed",     "speed");
        presetCombo->setCurrentIndex(1); // default: balanced
        presetCombo->setToolTip("AMD AMF preset.\nquality = slowest, best compression\nspeed = fastest");

    } else if (encoder == "libsvtav1") {
        // SVT-AV1: 0 (slowest/best) – 13 (fastest), key HandBrake-aligned points shown
        add("0 (quality)",   "0");
        add("2",             "2");
        add("4",             "4");
        add("5 (balanced)",  "5");
        add("7",             "7");
        add("10",            "10");
        add("12 (speed)",    "12");
        add("13",            "13");
        presetCombo->setCurrentIndex(3); // default: balanced (5)
        presetCombo->setToolTip("SVT-AV1 preset (0–13).\n0 = quality (slowest)\n5 = balanced\n12–13 = speed (fastest)");

    } else {
        // Software encoders (libx265, libx264): standard FFmpeg preset names
        add("veryslow (quality)", "veryslow");
        add("slower",             "slower");
        add("slow",               "slow");
        add("medium (balanced)",  "medium");
        add("fast",               "fast");
        add("faster",             "faster");
        add("veryfast",           "veryfast");
        add("superfast",          "superfast");
        add("ultrafast (speed)",  "ultrafast");
        presetCombo->setCurrentIndex(3); // default: medium
        presetCombo->setToolTip("x264/x265 preset.\nveryslow = quality (slowest)\nmedium = balanced\nultrafast = speed (fastest)");
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
        // AV1
        "libsvtav1", "av1_qsv", "av1_nvenc", "av1_amf",
        // HEVC
        "libx265", "hevc_qsv", "hevc_nvenc", "hevc_amf",
        // H.264
        "libx264", "h264_qsv", "h264_nvenc", "h264_amf"
    });
    settingsLayout->addWidget(encoderCombo, 0, 1);

    QLabel *presetLabel = new QLabel("Preset:", this);
    presetLabel->setToolTip("Encoding speed vs quality preset.\n"
                            "Options change based on the selected encoder.");
    settingsLayout->addWidget(presetLabel, 1, 0);
    presetCombo = new QComboBox(this);
    settingsLayout->addWidget(presetCombo, 1, 1);

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

    // Populate presets for the initially selected encoder
    updatePresetOptions(encoderCombo->currentText());
    connect(encoderCombo, &QComboBox::currentTextChanged, this, &PredictTab::updatePresetOptions);

    // Connections
    connect(predBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Video", "", "Video Files (*.mp4 *.mkv *.mov *.webm *.avi)");
        if (!fileName.isEmpty()) predFileEdit->setText(fileName);
    });

    connect(predictCancelBtn, &QPushButton::clicked, this, [this]() {
        predictJob->cancel();
    });

    connect(predictRunBtn, &QPushButton::clicked, this, [this]() {
        QString inputFile = predFileEdit->text();
        if (inputFile.isEmpty() || !QFileInfo::exists(inputFile) || !VideoUtils::isValidVideoFile(inputFile)) {
            QMessageBox::warning(this, "Error", "Please select a valid input file.");
            return;
        }

        m_pendingRunDetails = QString("%1 (%2, preset %3)")
            .arg(QFileInfo(inputFile).fileName())
            .arg(encoderCombo->currentText())
            .arg(presetCombo->currentData().toString());

        predictRunBtn->setEnabled(false);
        predictRunBtn->setText("Running ab-av1...");
        predictOutput->clear();
        predictOutput->append("Starting CRF search...");
        predictProgressBar->setValue(0);
        predictProgressBar->setVisible(true);
        predResultsGroup->setVisible(false);
        predictCancelBtn->setEnabled(true);

        predictJob->start(inputFile,
                          encoderCombo->currentText(),
                          presetCombo->currentData().toString(),
                          vmafSpin->value(),
                          samplesSpin->value());
    });
}