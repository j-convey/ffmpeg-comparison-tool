#include "VerifyTab.h"
#include "VideoUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>
#include <QFileInfo>
#include <QTime>

VerifyTab::VerifyTab(QWidget *parent) : QWidget(parent) {
    ffmpegJob = new FfmpegJob(this);
    setupUI();

    // Log lines → output widget
    connect(ffmpegJob, &FfmpegJob::logLine, this, [this](const QString& line) {
        outputText->append(line);
        outputText->verticalScrollBar()->setValue(outputText->verticalScrollBar()->maximum());
    });

    // Progress → progress bar
    connect(ffmpegJob, &FfmpegJob::progressUpdated, this, [this](double current, double total) {
        int pct = qMin(100, static_cast<int>((current / total) * 100.0));
        progressBar->setValue(pct);
        progressBar->setFormat(QString("%1% - %2 / %3")
            .arg(pct)
            .arg(QTime(0,0,0).addSecs(static_cast<int>(current)).toString("HH:mm:ss"))
            .arg(QTime(0,0,0).addSecs(static_cast<int>(total)).toString("HH:mm:ss")));
    });

    // SSIM result → update labels with color coding
    connect(ffmpegJob, &FfmpegJob::ssimResult, this, [this](const SsimResult& r) {
        auto style = [](double s) {
            QString b = "QLabel { font-size: 12pt; font-weight: bold; padding: 8px; border-radius: 5px; ";
            if (s >= 0.99) return b + "background-color: #4caf50; color: white; }";
            if (s >= 0.95) return b + "background-color: #8bc34a; color: white; }";
            if (s >= 0.90) return b + "background-color: #ffeb3b; color: black; }";
            if (s >= 0.80) return b + "background-color: #ff9800; color: white; }";
            return b + "background-color: #f44336; color: white; }";
        };
        auto fmt = [](double v, const QString& db) {
            return QString("%1\n(%2)").arg(v, 0, 'f', 4).arg(db == "inf" ? "∞ dB" : db + " dB");
        };
        resultYLabel  ->setText("Y: "       + fmt(r.y,   r.yDb));   resultYLabel  ->setStyleSheet(style(r.y));
        resultULabel  ->setText("U: "       + fmt(r.u,   r.uDb));   resultULabel  ->setStyleSheet(style(r.u));
        resultVLabel  ->setText("V: "       + fmt(r.v,   r.vDb));   resultVLabel  ->setStyleSheet(style(r.v));
        resultAllLabel->setText("Overall: " + fmt(r.all, r.allDb)); resultAllLabel->setStyleSheet(style(r.all));
        resultsGroup->setVisible(true);
    });

    // PSNR result → update labels with color coding
    connect(ffmpegJob, &FfmpegJob::psnrResult, this, [this](const PsnrResult& r) {
        auto style = [](const QString& db) {
            if (db == "inf") return QString(
                "QLabel { font-size: 12pt; font-weight: bold; padding: 8px; "
                "border-radius: 5px; background-color: #4caf50; color: white; }");
            QString b = "QLabel { font-size: 12pt; font-weight: bold; padding: 8px; border-radius: 5px; ";
            double v = db.toDouble();
            if (v >= 40.0) return b + "background-color: #4caf50; color: white; }";
            if (v >= 35.0) return b + "background-color: #8bc34a; color: white; }";
            if (v >= 30.0) return b + "background-color: #ffeb3b; color: black; }";
            if (v >= 25.0) return b + "background-color: #ff9800; color: white; }";
            return b + "background-color: #f44336; color: white; }";
        };
        auto fmt = [](const QString& db) { return db == "inf" ? QString("∞") : db; };
        psnrYLabel  ->setText(QString("Y: %1 dB")  .arg(fmt(r.yDb)));  psnrYLabel  ->setStyleSheet(style(r.yDb));
        psnrULabel  ->setText(QString("U: %1 dB")  .arg(fmt(r.uDb)));  psnrULabel  ->setStyleSheet(style(r.uDb));
        psnrVLabel  ->setText(QString("V: %1 dB")  .arg(fmt(r.vDb)));  psnrVLabel  ->setStyleSheet(style(r.vDb));
        psnrAvgLabel->setText(QString("Avg: %1 dB").arg(fmt(r.avgDb))); psnrAvgLabel->setStyleSheet(style(r.avgDb));
        resultsGroup->setVisible(true);
    });

    // VMAF result → update label with color coding
    connect(ffmpegJob, &FfmpegJob::vmafResult, this, [this](double score) {
        QString style = "QLabel { font-size: 14pt; font-weight: bold; padding: 12px; border-radius: 5px; ";
        if      (score >= 95.0) style += "background-color: #4caf50; color: white; }";
        else if (score >= 85.0) style += "background-color: #8bc34a; color: white; }";
        else if (score >= 75.0) style += "background-color: #ffeb3b; color: black; }";
        else if (score >= 60.0) style += "background-color: #ff9800; color: white; }";
        else                    style += "background-color: #f44336; color: white; }";
        vmafScoreLabel->setText(QString("VMAF: %1").arg(score, 0, 'f', 2));
        vmafScoreLabel->setStyleSheet(style);
        resultsGroup->setVisible(true);
    });

    // Finished → restore UI and emit history signal
    connect(ffmpegJob, &FfmpegJob::finished, this, [this](bool success, int exitCode) {
        runBtn->setEnabled(true);
        runBtn->setText("Run Comparison");
        progressBar->setVisible(false);
        outputText->append("\n" + QString("-").repeated(80));
        if (success) {
            outputText->append("\nComparison completed successfully!");
            QString details = QFileInfo(originalFileEdit->text()).fileName() + " vs " +
                              QFileInfo(comparisonFileEdit->text()).fileName();
            QStringList results;
            if (resultAllLabel->text() != "Overall: --")
                results << "SSIM: " + resultAllLabel->text().replace("\n", " ");
            if (psnrAvgLabel->text() != "Average: --") results << "PSNR: " + psnrAvgLabel->text();
            if (vmafScoreLabel->text() != "VMAF Score: --") results << vmafScoreLabel->text();
            emit comparisonCompleted("Comparison", details, results.join(" | "));
        } else {
            outputText->append(QString("\nFFmpeg exited with code: %1").arg(exitCode));
        }
        outputText->verticalScrollBar()->setValue(outputText->verticalScrollBar()->maximum());
    });
}

void VerifyTab::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // File Selection Group
    QGroupBox *fileGroup = new QGroupBox("File Selection", this);
    QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);
    
    // Original file
    QHBoxLayout *originalLayout = new QHBoxLayout();
    QLabel *originalLabel = new QLabel("Original Media:", this);
    originalLabel->setMinimumWidth(120);
    originalFileEdit = new QLineEdit(this);
    originalFileEdit->setReadOnly(true);
    originalFileBtn = new QPushButton("Browse...", this);
    originalResolutionLabel = new QLabel("", this);
    originalResolutionLabel->setStyleSheet("QLabel { color: #0066cc; font-weight: bold; }");
    originalResolutionLabel->setMinimumWidth(100);
    originalLayout->addWidget(originalLabel);
    originalLayout->addWidget(originalFileEdit);
    originalLayout->addWidget(originalResolutionLabel);
    originalLayout->addWidget(originalFileBtn);
    fileLayout->addLayout(originalLayout);
    
    // Comparison file
    QHBoxLayout *comparisonLayout = new QHBoxLayout();
    QLabel *comparisonLabel = new QLabel("Comparison Media:", this);
    comparisonLabel->setMinimumWidth(120);
    comparisonFileEdit = new QLineEdit(this);
    comparisonFileEdit->setReadOnly(true);
    comparisonFileBtn = new QPushButton("Browse...", this);
    comparisonResolutionLabel = new QLabel("", this);
    comparisonResolutionLabel->setStyleSheet("QLabel { color: #0066cc; font-weight: bold; }");
    comparisonResolutionLabel->setMinimumWidth(100);
    comparisonLayout->addWidget(comparisonLabel);
    comparisonLayout->addWidget(comparisonFileEdit);
    comparisonLayout->addWidget(comparisonResolutionLabel);
    comparisonLayout->addWidget(comparisonFileBtn);
    fileLayout->addLayout(comparisonLayout);
    
    mainLayout->addWidget(fileGroup);
    
    // Time Options Group
    QGroupBox *timeGroup = new QGroupBox("Time Options", this);
    QVBoxLayout *timeLayout = new QVBoxLayout(timeGroup);
    
    // Start time
    QHBoxLayout *startLayout = new QHBoxLayout();
    useStartTimeCheckbox = new QCheckBox("Start Time (HH:MM:SS):", this);
    startTimeEdit = new QLineEdit("00:00:00", this);
    startTimeEdit->setMaximumWidth(150);
    startTimeEdit->setEnabled(false);
    startLayout->addWidget(useStartTimeCheckbox);
    startLayout->addWidget(startTimeEdit);
    startLayout->addStretch();
    timeLayout->addLayout(startLayout);
    
    // Duration
    QHBoxLayout *durationLayout = new QHBoxLayout();
    useDurationCheckbox = new QCheckBox("Duration (HH:MM:SS):", this);
    durationEdit = new QLineEdit("00:01:00", this);
    durationEdit->setMaximumWidth(150);
    durationEdit->setEnabled(false);
    durationLayout->addWidget(useDurationCheckbox);
    durationLayout->addWidget(durationEdit);
    durationLayout->addStretch();
    timeLayout->addLayout(durationLayout);
    
    mainLayout->addWidget(timeGroup);
    
    // Run button
    runBtn = new QPushButton("Run Comparison", this);
    runBtn->setMinimumHeight(40);
    mainLayout->addWidget(runBtn);
    
    // Progress bar
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    mainLayout->addWidget(progressBar);
    
    // Results Group
    resultsGroup = new QGroupBox("Comprehensive Quality Analysis Results", this);
    resultsGroup->setVisible(false);
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsGroup);
    
    // SSIM Section
    QLabel *ssimHeader = new QLabel("<b>SSIM (Structural Similarity Index)</b> - Perceptual similarity (0-1 scale, 1 = identical)", this);
    ssimHeader->setStyleSheet("QLabel { color: #555; font-size: 9pt; padding: 5px; background-color: #e3f2fd; border-radius: 3px; margin-top: 5px; }");
    resultsLayout->addWidget(ssimHeader);
    
    // SSIM Results display
    QHBoxLayout *ssimLayout = new QHBoxLayout();
    
    resultYLabel = new QLabel("Y: --", this);
    resultYLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; background-color: #e3f2fd; border-radius: 5px; }");
    resultYLabel->setAlignment(Qt::AlignCenter);
    resultYLabel->setToolTip("Luminance (Brightness) Similarity");
    ssimLayout->addWidget(resultYLabel);
    
    resultULabel = new QLabel("U: --", this);
    resultULabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; background-color: #e3f2fd; border-radius: 5px; }");
    resultULabel->setAlignment(Qt::AlignCenter);
    resultULabel->setToolTip("U Chrominance (Color) Similarity");
    ssimLayout->addWidget(resultULabel);
    
    resultVLabel = new QLabel("V: --", this);
    resultVLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; background-color: #e3f2fd; border-radius: 5px; }");
    resultVLabel->setAlignment(Qt::AlignCenter);
    resultVLabel->setToolTip("V Chrominance (Color) Similarity");
    ssimLayout->addWidget(resultVLabel);
    
    resultAllLabel = new QLabel("Overall: --", this);
    resultAllLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; background-color: #bbdefb; border-radius: 5px; }");
    resultAllLabel->setAlignment(Qt::AlignCenter);
    resultAllLabel->setToolTip("Overall SSIM Score");
    ssimLayout->addWidget(resultAllLabel);
    
    resultsLayout->addLayout(ssimLayout);
    
    // PSNR Section
    QLabel *psnrHeader = new QLabel("<b>PSNR (Peak Signal-to-Noise Ratio)</b> - Quality in dB (higher is better, >40dB = excellent)", this);
    psnrHeader->setStyleSheet("QLabel { color: #555; font-size: 9pt; padding: 5px; background-color: #e8f5e9; border-radius: 3px; margin-top: 10px; }");
    resultsLayout->addWidget(psnrHeader);
    
    // PSNR Results display
    QHBoxLayout *psnrLayout = new QHBoxLayout();
    
    psnrYLabel = new QLabel("Y: --", this);
    psnrYLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; background-color: #e8f5e9; border-radius: 5px; }");
    psnrYLabel->setAlignment(Qt::AlignCenter);
    psnrYLabel->setToolTip("Luminance PSNR");
    psnrLayout->addWidget(psnrYLabel);
    
    psnrULabel = new QLabel("U: --", this);
    psnrULabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; background-color: #e8f5e9; border-radius: 5px; }");
    psnrULabel->setAlignment(Qt::AlignCenter);
    psnrULabel->setToolTip("U Chrominance PSNR");
    psnrLayout->addWidget(psnrULabel);
    
    psnrVLabel = new QLabel("V: --", this);
    psnrVLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; background-color: #e8f5e9; border-radius: 5px; }");
    psnrVLabel->setAlignment(Qt::AlignCenter);
    psnrVLabel->setToolTip("V Chrominance PSNR");
    psnrLayout->addWidget(psnrVLabel);
    
    psnrAvgLabel = new QLabel("Average: --", this);
    psnrAvgLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; background-color: #c8e6c9; border-radius: 5px; }");
    psnrAvgLabel->setAlignment(Qt::AlignCenter);
    psnrAvgLabel->setToolTip("Average PSNR");
    psnrLayout->addWidget(psnrAvgLabel);
    
    resultsLayout->addLayout(psnrLayout);
    
    // VMAF Section
    QLabel *vmafHeader = new QLabel("<b>VMAF (Video Multimethod Assessment Fusion)</b> - Netflix quality metric (0-100 scale, >95 = excellent)", this);
    vmafHeader->setStyleSheet("QLabel { color: #555; font-size: 9pt; padding: 5px; background-color: #fff3e0; border-radius: 3px; margin-top: 10px; }");
    resultsLayout->addWidget(vmafHeader);
    
    // VMAF Results display
    QHBoxLayout *vmafLayout = new QHBoxLayout();
    
    vmafScoreLabel = new QLabel("VMAF Score: --", this);
    vmafScoreLabel->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; padding: 12px; background-color: #ffe0b2; border-radius: 5px; }");
    vmafScoreLabel->setAlignment(Qt::AlignCenter);
    vmafScoreLabel->setToolTip("Overall VMAF Quality Score");
    vmafLayout->addWidget(vmafScoreLabel);
    
    resultsLayout->addLayout(vmafLayout);
    mainLayout->addWidget(resultsGroup);
    
    // Add spacing
    mainLayout->addSpacing(10);
    
    // Output area (smaller, at bottom)
    QLabel *outputLabel = new QLabel("Detailed Output Log:", this);
    outputLabel->setStyleSheet("QLabel { font-weight: bold; }");
    mainLayout->addWidget(outputLabel);
    outputText = new QTextEdit(this);
    outputText->setReadOnly(true);
    outputText->setFont(QFont("Courier New", 8));
    outputText->setMaximumHeight(200);  // Limit output height
    mainLayout->addWidget(outputText);
    
    // Connect signals
    connect(originalFileBtn, &QPushButton::clicked, this, &VerifyTab::selectOriginalFile);
    connect(comparisonFileBtn, &QPushButton::clicked, this, &VerifyTab::selectComparisonFile);
    connect(runBtn, &QPushButton::clicked, this, &VerifyTab::runComparison);
    connect(useStartTimeCheckbox, &QCheckBox::toggled, startTimeEdit, &QLineEdit::setEnabled);
    connect(useDurationCheckbox, &QCheckBox::toggled, durationEdit, &QLineEdit::setEnabled);
}

void VerifyTab::selectOriginalFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select Original Media File",
        "",
        "Video Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv);;All Files (*.*)");
    
    if (!fileName.isEmpty()) {
        if (!VideoUtils::isValidVideoFile(fileName)) {
            QMessageBox::warning(this, "Invalid File", 
                "Please select a valid video file (MP4, AVI, MKV, MOV, WMV, or FLV).");
            return;
        }
        
        originalFileEdit->setText(fileName);
        
        // Get and display resolution
        QString resolution = VideoUtils::getVideoResolution(fileName);
        if (!resolution.isEmpty()) {
            originalResolutionLabel->setText(resolution);
        } else {
            originalResolutionLabel->setText("(unknown)");
        }
    }
}

void VerifyTab::selectComparisonFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select Comparison Media File",
        "",
        "Video Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv);;All Files (*.*)");
    
    if (!fileName.isEmpty()) {
        if (!VideoUtils::isValidVideoFile(fileName)) {
            QMessageBox::warning(this, "Invalid File", 
                "Please select a valid video file (MP4, AVI, MKV, MOV, WMV, or FLV).");
            return;
        }
        
        comparisonFileEdit->setText(fileName);
        
        // Get and display resolution
        QString resolution = VideoUtils::getVideoResolution(fileName);
        if (!resolution.isEmpty()) {
            comparisonResolutionLabel->setText(resolution);
        } else {
            comparisonResolutionLabel->setText("(unknown)");
        }
    }
}

bool VerifyTab::validateInputs() {
    if (originalFileEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please select an original media file.");
        return false;
    }
    
    if (comparisonFileEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please select a comparison media file.");
        return false;
    }
    
    // Validate time format
    QRegularExpression timeRegex("^\\d{2}:\\d{2}:\\d{2}$");
    
    if (useStartTimeCheckbox->isChecked()) {
        if (!timeRegex.match(startTimeEdit->text()).hasMatch()) {
            QMessageBox::warning(this, "Validation Error", 
                "Start time must be in HH:MM:SS format (e.g., 00:08:00).");
            return false;
        }
    }
    
    if (useDurationCheckbox->isChecked()) {
        if (!timeRegex.match(durationEdit->text()).hasMatch()) {
            QMessageBox::warning(this, "Validation Error", 
                "Duration must be in HH:MM:SS format (e.g., 00:03:00).");
            return false;
        }
    }
    
    return true;
}

void VerifyTab::runComparison() {
    if (!validateInputs()) return;

    outputText->clear();
    runBtn->setEnabled(false);
    runBtn->setText("Running...");
    progressBar->setValue(0);
    progressBar->setVisible(true);
    resultsGroup->setVisible(false);

    ffmpegJob->start(
        originalFileEdit->text(),
        comparisonFileEdit->text(),
        useStartTimeCheckbox->isChecked() ? startTimeEdit->text() : QString(),
        useDurationCheckbox ->isChecked() ? durationEdit ->text() : QString()
    );
}