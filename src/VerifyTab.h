#ifndef VERIFYTAB_H
#define VERIFYTAB_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QProgressBar>
#include <QGroupBox>
#include <QTextEdit>
#include "FfmpegJob.h"

class VerifyTab : public QWidget {
    Q_OBJECT

public:
    explicit VerifyTab(QWidget *parent = nullptr);

signals:
    void comparisonCompleted(const QString& type, const QString& details, const QString& result);

private slots:
    void selectOriginalFile();
    void selectComparisonFile();
    void runComparison();

private:
    void setupUI();
    bool validateInputs();

    QLineEdit *originalFileEdit;
    QLineEdit *comparisonFileEdit;
    QPushButton *originalFileBtn;
    QPushButton *comparisonFileBtn;
    QLabel *originalResolutionLabel;
    QLabel *comparisonResolutionLabel;
    
    QCheckBox *useStartTimeCheckbox;
    QLineEdit *startTimeEdit;
    QCheckBox *useDurationCheckbox;
    QLineEdit *durationEdit;
    
    QPushButton *runBtn;
    QProgressBar *progressBar;
    QGroupBox *resultsGroup;
    
    // Result Labels (Y, U, V, All, PSNR, VMAF)
    QLabel *resultYLabel, *resultULabel, *resultVLabel, *resultAllLabel;
    QLabel *psnrYLabel, *psnrULabel, *psnrVLabel, *psnrAvgLabel;
    QLabel *vmafScoreLabel;
    
    QTextEdit *outputText;
    FfmpegJob *ffmpegJob;
};

#endif // VERIFYTAB_H