#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QRegularExpression>
#include <QScrollBar>
#include <QFileInfo>
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ffmpegProcess(nullptr) {
    setupUI();
    setWindowTitle("FFmpeg Video Comparison Tool (SSIM + PSNR + VMAF)");
    resize(950, 850);  // Larger window for additional metrics
}

MainWindow::~MainWindow() {
    if (ffmpegProcess) {
        ffmpegProcess->kill();
        ffmpegProcess->waitForFinished();
        delete ffmpegProcess;
    }
}

void MainWindow::setupUI() {
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setTabPosition(QTabWidget::West);

    QWidget *predictTab = new QWidget(this);
    QWidget *verifyTab = new QWidget(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(verifyTab);
    
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
    
    tabWidget->addTab(predictTab, "Predict");
    tabWidget->addTab(verifyTab, "Verify");
    setCentralWidget(tabWidget);
    
    // Connect signals
    connect(originalFileBtn, &QPushButton::clicked, this, &MainWindow::selectOriginalFile);
    connect(comparisonFileBtn, &QPushButton::clicked, this, &MainWindow::selectComparisonFile);
    connect(runBtn, &QPushButton::clicked, this, &MainWindow::runComparison);
    connect(useStartTimeCheckbox, &QCheckBox::toggled, startTimeEdit, &QLineEdit::setEnabled);
    connect(useDurationCheckbox, &QCheckBox::toggled, durationEdit, &QLineEdit::setEnabled);
}

void MainWindow::selectOriginalFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select Original Media File",
        "",
        "Video Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv);;All Files (*.*)");
    
    if (!fileName.isEmpty()) {
        if (!isValidVideoFile(fileName)) {
            QMessageBox::warning(this, "Invalid File", 
                "Please select a valid video file (MP4, AVI, MKV, MOV, WMV, or FLV).");
            return;
        }
        
        originalFileEdit->setText(fileName);
        
        // Get and display resolution
        QString resolution = getVideoResolution(fileName);
        if (!resolution.isEmpty()) {
            originalResolutionLabel->setText(resolution);
        } else {
            originalResolutionLabel->setText("(unknown)");
        }
    }
}

void MainWindow::selectComparisonFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select Comparison Media File",
        "",
        "Video Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv);;All Files (*.*)");
    
    if (!fileName.isEmpty()) {
        if (!isValidVideoFile(fileName)) {
            QMessageBox::warning(this, "Invalid File", 
                "Please select a valid video file (MP4, AVI, MKV, MOV, WMV, or FLV).");
            return;
        }
        
        comparisonFileEdit->setText(fileName);
        
        // Get and display resolution
        QString resolution = getVideoResolution(fileName);
        if (!resolution.isEmpty()) {
            comparisonResolutionLabel->setText(resolution);
        } else {
            comparisonResolutionLabel->setText("(unknown)");
        }
    }
}

bool MainWindow::validateInputs() {
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

void MainWindow::runComparison() {
    if (!validateInputs()) {
        return;
    }
    
    // Build FFmpeg command
    QStringList arguments;
    
    // Add start time and duration for first input if specified
    if (useStartTimeCheckbox->isChecked()) {
        arguments << "-ss" << startTimeEdit->text();
    }
    if (useDurationCheckbox->isChecked()) {
        arguments << "-t" << durationEdit->text();
    }
    
    // First input file
    arguments << "-i" << originalFileEdit->text();
    
    // Add start time and duration for second input if specified
    if (useStartTimeCheckbox->isChecked()) {
        arguments << "-ss" << startTimeEdit->text();
    }
    if (useDurationCheckbox->isChecked()) {
        arguments << "-t" << durationEdit->text();
    }
    
    // Second input file
    arguments << "-i" << comparisonFileEdit->text();
    
    // Complex filter with SSIM, PSNR, and VMAF
    QString filterComplex = "[0:v]split=3[ref1][ref2][ref3];[1:v]split=3[main1][main2][main3];"
                            "[main1][ref1]ssim[stats_ssim];"
                            "[main2][ref2]psnr[stats_psnr];"
                            "[main3][ref3]libvmaf";
    
    arguments << "-filter_complex" << filterComplex
              << "-map" << "[stats_ssim]"
              << "-map" << "[stats_psnr]"
              << "-f" << "null" << "-";
    
    // Display command
    outputText->clear();
    outputText->append("Running command:");
    outputText->append("ffmpeg " + arguments.join(" "));
    outputText->append("\n" + QString("-").repeated(80) + "\n");
    
    // Create and configure process
    if (ffmpegProcess) {
        delete ffmpegProcess;
    }
    
    ffmpegProcess = new QProcess(this);
    connect(ffmpegProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::processOutput);
    connect(ffmpegProcess, &QProcess::readyReadStandardError, this, &MainWindow::processError);
    connect(ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::processFinished);
    
    // Disable run button while processing
    runBtn->setEnabled(false);
    runBtn->setText("Running...");
    
    // Initialize progress tracking
    totalDuration = 0.0;
    currentTime = 0.0;
    progressBar->setValue(0);
    progressBar->setVisible(true);
    resultsGroup->setVisible(false);
    
    // Start FFmpeg
    ffmpegProcess->start("ffmpeg", arguments);
    
    if (!ffmpegProcess->waitForStarted()) {
        QMessageBox::critical(this, "Error", 
            "Failed to start FFmpeg. Please ensure FFmpeg is installed and in your PATH.");
        runBtn->setEnabled(true);
        runBtn->setText("Run Comparison");
    }
}

void MainWindow::processOutput() {
    if (ffmpegProcess) {
        QString output = QString::fromLocal8Bit(ffmpegProcess->readAllStandardOutput());
        outputText->append(output);
    }
}

void MainWindow::processError() {
    if (ffmpegProcess) {
        QString error = QString::fromLocal8Bit(ffmpegProcess->readAllStandardError());
        outputText->append(error);
        
        // Parse duration from input file info if not set
        if (totalDuration == 0.0 && !useDurationCheckbox->isChecked()) {
            QRegularExpression durationRegex("Duration: (\\d{2}):(\\d{2}):(\\d{2})\\.(\\d{2})");
            QRegularExpressionMatch match = durationRegex.match(error);
            if (match.hasMatch()) {
                int hours = match.captured(1).toInt();
                int minutes = match.captured(2).toInt();
                int seconds = match.captured(3).toInt();
                totalDuration = hours * 3600.0 + minutes * 60.0 + seconds;
            }
        }
        
        // If duration was specified, use that
        if (totalDuration == 0.0 && useDurationCheckbox->isChecked()) {
            QStringList parts = durationEdit->text().split(":");
            if (parts.size() == 3) {
                totalDuration = parts[0].toInt() * 3600.0 + parts[1].toInt() * 60.0 + parts[2].toInt();
            }
        }
        
        // Parse progress from frame lines
        QRegularExpression progressRegex("time=(\\d{2}):(\\d{2}):(\\d{2})\\.(\\d{2})");
        QRegularExpressionMatch progressMatch = progressRegex.match(error);
        if (progressMatch.hasMatch() && totalDuration > 0.0) {
            int hours = progressMatch.captured(1).toInt();
            int minutes = progressMatch.captured(2).toInt();
            int seconds = progressMatch.captured(3).toInt();
            currentTime = hours * 3600.0 + minutes * 60.0 + seconds;
            
            int percentage = qMin(100, static_cast<int>((currentTime / totalDuration) * 100.0));
            progressBar->setValue(percentage);
            progressBar->setFormat(QString("%1% - %2 / %3")
                .arg(percentage)
                .arg(QTime(0, 0, 0).addSecs(static_cast<int>(currentTime)).toString("HH:mm:ss"))
                .arg(QTime(0, 0, 0).addSecs(static_cast<int>(totalDuration)).toString("HH:mm:ss")));
        }
        
        // Parse SSIM results
        QRegularExpression ssimRegex("SSIM Y:([\\d.]+) \\(([\\d.]+|inf)\\) U:([\\d.]+) \\(([\\d.]+|inf)\\) V:([\\d.]+) \\(([\\d.]+|inf)\\) All:([\\d.]+) \\(([\\d.]+|inf)\\)");
        QRegularExpressionMatch ssimMatch = ssimRegex.match(error);
        if (ssimMatch.hasMatch()) {
            double yScore = ssimMatch.captured(1).toDouble();
            QString yDbStr = ssimMatch.captured(2);
            double uScore = ssimMatch.captured(3).toDouble();
            QString uDbStr = ssimMatch.captured(4);
            double vScore = ssimMatch.captured(5).toDouble();
            QString vDbStr = ssimMatch.captured(6);
            double allScore = ssimMatch.captured(7).toDouble();
            QString allDbStr = ssimMatch.captured(8);
            
            // Update result labels with color coding
            auto getSSIMColorStyle = [](double score) {
                QString baseStyle = "QLabel { font-size: 12pt; font-weight: bold; padding: 8px; border-radius: 5px; ";
                if (score >= 0.99) return baseStyle + "background-color: #4caf50; color: white; }"; // Excellent - green
                if (score >= 0.95) return baseStyle + "background-color: #8bc34a; color: white; }"; // Very good - light green
                if (score >= 0.90) return baseStyle + "background-color: #ffeb3b; color: black; }"; // Good - yellow
                if (score >= 0.80) return baseStyle + "background-color: #ff9800; color: white; }"; // Fair - orange
                return baseStyle + "background-color: #f44336; color: white; }"; // Poor - red
            };
            
            resultYLabel->setText(QString("Y: %1\n(%2)").arg(yScore, 0, 'f', 4).arg(yDbStr == "inf" ? "∞ dB" : yDbStr + " dB"));
            resultYLabel->setStyleSheet(getSSIMColorStyle(yScore));
            
            resultULabel->setText(QString("U: %1\n(%2)").arg(uScore, 0, 'f', 4).arg(uDbStr == "inf" ? "∞ dB" : uDbStr + " dB"));
            resultULabel->setStyleSheet(getSSIMColorStyle(uScore));
            
            resultVLabel->setText(QString("V: %1\n(%2)").arg(vScore, 0, 'f', 4).arg(vDbStr == "inf" ? "∞ dB" : vDbStr + " dB"));
            resultVLabel->setStyleSheet(getSSIMColorStyle(vScore));
            
            resultAllLabel->setText(QString("Overall: %1\n(%2)").arg(allScore, 0, 'f', 4).arg(allDbStr == "inf" ? "∞ dB" : allDbStr + " dB"));
            resultAllLabel->setStyleSheet(getSSIMColorStyle(allScore));
            
            resultsGroup->setVisible(true);
        }
        
        // Parse PSNR results
        QRegularExpression psnrRegex("PSNR.*?y:([\\d.]+|inf).*?u:([\\d.]+|inf).*?v:([\\d.]+|inf).*?average:([\\d.]+|inf)");
        QRegularExpressionMatch psnrMatch = psnrRegex.match(error);
        if (psnrMatch.hasMatch()) {
            QString yDb = psnrMatch.captured(1);
            QString uDb = psnrMatch.captured(2);
            QString vDb = psnrMatch.captured(3);
            QString avgDb = psnrMatch.captured(4);
            
            // Update PSNR labels with color coding
            auto getPSNRColorStyle = [](const QString& dbStr) {
                if (dbStr == "inf") {
                    return QString("QLabel { font-size: 12pt; font-weight: bold; padding: 8px; border-radius: 5px; background-color: #4caf50; color: white; }");
                }
                double db = dbStr.toDouble();
                QString baseStyle = "QLabel { font-size: 12pt; font-weight: bold; padding: 8px; border-radius: 5px; ";
                if (db >= 40.0) return baseStyle + "background-color: #4caf50; color: white; }"; // Excellent
                if (db >= 35.0) return baseStyle + "background-color: #8bc34a; color: white; }"; // Very good
                if (db >= 30.0) return baseStyle + "background-color: #ffeb3b; color: black; }"; // Good
                if (db >= 25.0) return baseStyle + "background-color: #ff9800; color: white; }"; // Fair
                return baseStyle + "background-color: #f44336; color: white; }"; // Poor
            };
            
            psnrYLabel->setText(QString("Y: %1 dB").arg(yDb == "inf" ? "∞" : yDb));
            psnrYLabel->setStyleSheet(getPSNRColorStyle(yDb));
            
            psnrULabel->setText(QString("U: %1 dB").arg(uDb == "inf" ? "∞" : uDb));
            psnrULabel->setStyleSheet(getPSNRColorStyle(uDb));
            
            psnrVLabel->setText(QString("V: %1 dB").arg(vDb == "inf" ? "∞" : vDb));
            psnrVLabel->setStyleSheet(getPSNRColorStyle(vDb));
            
            psnrAvgLabel->setText(QString("Avg: %1 dB").arg(avgDb == "inf" ? "∞" : avgDb));
            psnrAvgLabel->setStyleSheet(getPSNRColorStyle(avgDb));
            
            resultsGroup->setVisible(true);
        }
        
        // Parse VMAF results
        QRegularExpression vmafRegex("VMAF score[:\\s=]+([\\d.]+)");
        QRegularExpressionMatch vmafMatch = vmafRegex.match(error);
        if (vmafMatch.hasMatch()) {
            double vmafScore = vmafMatch.captured(1).toDouble();
            
            // Update VMAF label with color coding
            QString vmafStyle = "QLabel { font-size: 14pt; font-weight: bold; padding: 12px; border-radius: 5px; ";
            if (vmafScore >= 95.0) vmafStyle += "background-color: #4caf50; color: white; }"; // Excellent
            else if (vmafScore >= 85.0) vmafStyle += "background-color: #8bc34a; color: white; }"; // Very good
            else if (vmafScore >= 75.0) vmafStyle += "background-color: #ffeb3b; color: black; }"; // Good
            else if (vmafScore >= 60.0) vmafStyle += "background-color: #ff9800; color: white; }"; // Fair
            else vmafStyle += "background-color: #f44336; color: white; }"; // Poor
            
            vmafScoreLabel->setText(QString("VMAF: %1").arg(vmafScore, 0, 'f', 2));
            vmafScoreLabel->setStyleSheet(vmafStyle);
            
            resultsGroup->setVisible(true);
        }
    }
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    runBtn->setEnabled(true);
    runBtn->setText("Run Comparison");
    progressBar->setVisible(false);
    
    outputText->append("\n" + QString("-").repeated(80));
    
    if (exitStatus == QProcess::NormalExit) {
        if (exitCode == 0) {
            outputText->append("\nComparison completed successfully!");
        } else {
            outputText->append(QString("\nFFmpeg exited with code: %1").arg(exitCode));
        }
    } else {
        outputText->append("\nFFmpeg process crashed or was terminated.");
    }
    
    // Scroll to bottom
    outputText->verticalScrollBar()->setValue(outputText->verticalScrollBar()->maximum());
}

bool MainWindow::isValidVideoFile(const QString& filePath) {
    // Check file extension
    QStringList validExtensions = {"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "mpg", "mpeg", "m4v"};
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    return validExtensions.contains(extension);
}

QString MainWindow::getVideoResolution(const QString& filePath) {
    // Use ffprobe to get video resolution
    QProcess process;
    QStringList arguments;
    
    arguments << "-v" << "error"
              << "-select_streams" << "v:0"
              << "-show_entries" << "stream=width,height"
              << "-of" << "csv=s=x:p=0"
              << filePath;
    
    process.start("ffprobe", arguments);
    
    if (!process.waitForStarted(3000)) {
        return "";
    }
    
    if (!process.waitForFinished(5000)) {
        process.kill();
        return "";
    }
    
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    
    // Output should be in format "1920x1080"
    if (!output.isEmpty() && output.contains("x")) {
        return output;
    }
    
    return "";
}
