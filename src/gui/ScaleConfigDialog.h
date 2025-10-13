#ifndef SCALECONFIGDIALOG_H
#define SCALECONFIGDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QGroupBox>

/**
 * @brief Dialog para configuração de parâmetros de escala
 * 
 * Permite configurar:
 * - Distância entre cristas (mm)
 * - Cristas por 10mm
 * - Pixels por milímetro
 * - DPI
 */
class ScaleConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScaleConfigDialog(QWidget *parent = nullptr);
    ~ScaleConfigDialog();

    // Getters
    double getRidgeSpacing() const;
    double getRidgesPer10mm() const;
    double getPixelsPerMm() const;
    double getDPI() const;

    // Setters
    void setRidgeSpacing(double mm);
    void setRidgesPer10mm(double count);
    void setPixelsPerMm(double ppm);
    void setDPI(double dpi);

    // Modo de entrada
    enum InputMode {
        MODE_RIDGE_SPACING,    // Entrada: distância entre cristas
        MODE_RIDGES_PER_10MM,  // Entrada: número de cristas por 10mm
        MODE_PIXELS_PER_MM,    // Entrada: pixels por mm
        MODE_DPI               // Entrada: DPI
    };

    void setInputMode(InputMode mode);
    InputMode getInputMode() const { return currentMode; }

private slots:
    void onRidgeSpacingChanged(double value);
    void onRidgesPer10mmChanged(double value);
    void onPixelsPerMmChanged(double value);
    void onDPIChanged(double value);
    void resetToDefaults();

private:
    void setupUI();
    void updateCalculations();
    void blockSignalsTemporarily(bool block);

    // Widgets
    QDoubleSpinBox *ridgeSpacingSpinBox;
    QDoubleSpinBox *ridgesPer10mmSpinBox;
    QDoubleSpinBox *pixelsPerMmSpinBox;
    QDoubleSpinBox *dpiSpinBox;

    QLabel *infoLabel;
    QPushButton *resetButton;
    QPushButton *okButton;
    QPushButton *cancelButton;

    InputMode currentMode;
    bool updatingValues;  // Flag para evitar loops
};

#endif // SCALECONFIGDIALOG_H
