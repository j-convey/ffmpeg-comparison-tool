#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QCheckBox>
#include <QProcess>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectOriginalFile();
    void selectComparisonFile();
    void runComparison();
    void processOutput();
    void processError();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUI();
    QString formatTime(const QString& time);
    bool validateInputs();

    // UI Components
    QLineEdit *originalFileEdit;
    QLineEdit *comparisonFileEdit;
    QPushButton *originalFileBtn;
    QPushButton *comparisonFileBtn;
    
    QCheckBox *useStartTimeCheckbox;
    QLineEdit *startTimeEdit;
    
    QCheckBox *useDurationCheckbox;
    QLineEdit *durationEdit;
    
    QPushButton *runBtn;
    QProgressBar *progressBar;
    
    // Results display
    QGroupBox *resultsGroup;
    QLabel *resultYLabel;
    QLabel *resultULabel;
    QLabel *resultVLabel;
    QLabel *resultAllLabel;
    
    QTextEdit *outputText;
    
    QProcess *ffmpegProcess;
    double totalDuration;
    double currentTime;
};

#endif // MAINWINDOW_H
