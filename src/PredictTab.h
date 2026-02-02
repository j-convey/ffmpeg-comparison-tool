#ifndef PREDICTTAB_H
#define PREDICTTAB_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QProcess>

class PredictTab : public QWidget {
    Q_OBJECT

public:
    explicit PredictTab(QWidget *parent = nullptr);
    ~PredictTab();

signals:
    void predictionCompleted(const QString& type, const QString& details, const QString& result);

private:
    void setupUI();
    QString getAbAv1Path();

    QLineEdit *predFileEdit;
    QComboBox *encoderCombo;
    QLineEdit *presetEdit;
    QDoubleSpinBox *vmafSpin;
    QSpinBox *samplesSpin;
    
    QPushButton *predictRunBtn;
    QPushButton *predictCancelBtn;
    QProgressBar *predictProgressBar;
    
    QGroupBox *predResultsGroup;
    QLabel *predResultCRFLabel;
    QLabel *predResultVMAFLabel;
    QLabel *predResultSizeLabel;
    QLabel *predResultTimeLabel;
    
    QTextEdit *predictOutput;
    QProcess *predictProcess;
};

#endif // PREDICTTAB_H