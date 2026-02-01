#include "MainWindow.h"
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setWindowTitle("VidMetric - Video Comparison Tool (SSIM + PSNR + VMAF)");
    resize(950, 850);  // Larger window for additional metrics
}

void MainWindow::setupUI() {
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setTabPosition(QTabWidget::West);

    predictTab = new PredictTab(this);
    verifyTab = new VerifyTab(this);
    historyTab = new HistoryTab(this);
    
    tabWidget->addTab(predictTab, "Predict");
    tabWidget->addTab(verifyTab, "Verify");
    tabWidget->addTab(historyTab, "History");
    setCentralWidget(tabWidget);
    
    // Connect signals
    connect(predictTab, &PredictTab::predictionCompleted, historyTab, &HistoryTab::addEntry);
    connect(verifyTab, &VerifyTab::comparisonCompleted, historyTab, &HistoryTab::addEntry);
}
