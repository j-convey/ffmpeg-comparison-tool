#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QRegularExpression>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ffmpegProcess(nullptr) {
    setupUI();
    setWindowTitle("FFmpeg Video Comparison Tool (SSIM)");
    resize(900, 750);  // Taller window for better layout
}

MainWindow::~MainWindow() {
    if (ffmpegProcess) {
        ffmpegProcess->kill();
        ffmpegProcess->waitForFinished();
        delete ffmpegProcess;
    }
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
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
    originalLayout->addWidget(originalLabel);
    originalLayout->addWidget(originalFileEdit);
    originalLayout->addWidget(originalFileBtn);
    fileLayout->addLayout(originalLayout);
    
    // Comparison file
    QHBoxLayout *comparisonLayout = new QHBoxLayout();
    QLabel *comparisonLabel = new QLabel("Comparison Media:", this);
    comparisonLabel->setMinimumWidth(120);
    comparisonFileEdit = new QLineEdit(this);
    comparisonFileEdit->setReadOnly(true);
    comparisonFileBtn = new QPushButton("Browse...", this);
    comparisonLayout->addWidget(comparisonLabel);
    comparisonLayout->addWidget(comparisonFileEdit);
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
    resultsGroup = new QGroupBox("SSIM Comparison Results", this);
    resultsGroup->setVisible(false);
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsGroup);
    
    // Info label
    QLabel *infoLabel = new QLabel(
        "<b>SSIM (Structural Similarity Index):</b> Measures video quality similarity (0-1 scale, 1 = identical)<br>"
        "<b>Y (Luminance):</b> Brightness similarity | "
        "<b>U/V (Chrominance):</b> Color similarity | "
        "<b>All:</b> Overall quality",
        this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: #555; font-size: 9pt; padding: 5px; background-color: #f0f0f0; border-radius: 3px; }");
    resultsLayout->addWidget(infoLabel);
    
    // Results display
    QHBoxLayout *scoresLayout = new QHBoxLayout();
    
    resultYLabel = new QLabel("Y: --", this);
    resultYLabel->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; padding: 10px; background-color: #e3f2fd; border-radius: 5px; }");
    resultYLabel->setAlignment(Qt::AlignCenter);
    resultYLabel->setToolTip("Luminance (Brightness) Similarity");
    scoresLayout->addWidget(resultYLabel);
    
    resultULabel = new QLabel("U: --", this);
    resultULabel->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; padding: 10px; background-color: #e8f5e9; border-radius: 5px; }");
    resultULabel->setAlignment(Qt::AlignCenter);
    resultULabel->setToolTip("U Chrominance (Color) Similarity");
    scoresLayout->addWidget(resultULabel);
    
    resultVLabel = new QLabel("V: --", this);
    resultVLabel->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; padding: 10px; background-color: #fff3e0; border-radius: 5px; }");
    resultVLabel->setAlignment(Qt::AlignCenter);
    resultVLabel->setToolTip("V Chrominance (Color) Similarity");
    scoresLayout->addWidget(resultVLabel);
    
    resultAllLabel = new QLabel("Overall: --", this);
    resultAllLabel->setStyleSheet("QLabel { font-size: 16pt; font-weight: bold; padding: 10px; background-color: #f3e5f5; border-radius: 5px; }");
    resultAllLabel->setAlignment(Qt::AlignCenter);
    resultAllLabel->setToolTip("Overall Quality Score");
    scoresLayout->addWidget(resultAllLabel);
    
    resultsLayout->addLayout(scoresLayout);
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
    
    setCentralWidget(centralWidget);
    
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
        originalFileEdit->setText(fileName);
    }
}

void MainWindow::selectComparisonFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select Comparison Media File",
        "",
        "Video Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv);;All Files (*.*)");
    
    if (!fileName.isEmpty()) {
        comparisonFileEdit->setText(fileName);
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
    
    // SSIM filter and output
    arguments << "-lavfi" << "ssim" << "-f" << "null" << "-";
    
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
            auto getColorStyle = [](double score) {
                QString baseStyle = "QLabel { font-size: 14pt; font-weight: bold; padding: 10px; border-radius: 5px; ";
                if (score >= 0.99) return baseStyle + "background-color: #4caf50; color: white; }"; // Excellent - green
                if (score >= 0.95) return baseStyle + "background-color: #8bc34a; color: white; }"; // Very good - light green
                if (score >= 0.90) return baseStyle + "background-color: #ffeb3b; color: black; }"; // Good - yellow
                if (score >= 0.80) return baseStyle + "background-color: #ff9800; color: white; }"; // Fair - orange
                return baseStyle + "background-color: #f44336; color: white; }"; // Poor - red
            };
            
            resultYLabel->setText(QString("Y: %1\n(%2)").arg(yScore, 0, 'f', 4).arg(yDbStr == "inf" ? "∞ dB" : yDbStr + " dB"));
            resultYLabel->setStyleSheet(getColorStyle(yScore));
            
            resultULabel->setText(QString("U: %1\n(%2)").arg(uScore, 0, 'f', 4).arg(uDbStr == "inf" ? "∞ dB" : uDbStr + " dB"));
            resultULabel->setStyleSheet(getColorStyle(uScore));
            
            resultVLabel->setText(QString("V: %1\n(%2)").arg(vScore, 0, 'f', 4).arg(vDbStr == "inf" ? "∞ dB" : vDbStr + " dB"));
            resultVLabel->setStyleSheet(getColorStyle(vScore));
            
            QString overallStyle = "QLabel { font-size: 16pt; font-weight: bold; padding: 10px; border-radius: 5px; ";
            if (allScore >= 0.99) overallStyle += "background-color: #4caf50; color: white; }";
            else if (allScore >= 0.95) overallStyle += "background-color: #8bc34a; color: white; }";
            else if (allScore >= 0.90) overallStyle += "background-color: #ffeb3b; color: black; }";
            else if (allScore >= 0.80) overallStyle += "background-color: #ff9800; color: white; }";
            else overallStyle += "background-color: #f44336; color: white; }";
            
            resultAllLabel->setText(QString("Overall: %1\n(%2)").arg(allScore, 0, 'f', 4).arg(allDbStr == "inf" ? "∞ dB" : allDbStr + " dB"));
            resultAllLabel->setStyleSheet(overallStyle);
            
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
