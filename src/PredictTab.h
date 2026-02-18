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
#include "AbAv1Job.h"

class PredictTab : public QWidget {
    Q_OBJECT

public:
    explicit PredictTab(QWidget *parent = nullptr);

signals:
    void predictionCompleted(const QString& type, const QString& details, const QString& result);

private:
    void setupUI();
    void updatePresetOptions(const QString &encoder);

    QLineEdit *predFileEdit;
    QComboBox *encoderCombo;
    QComboBox *presetCombo;
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
    AbAv1Job  *predictJob;

    // Captured at job-start; used when the finished signal fires
    QString m_pendingRunDetails;
};

#endif // PREDICTTAB_H