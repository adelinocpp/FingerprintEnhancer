#ifndef MINUTIAEDISPLAYDIALOG_H
#define MINUTIAEDISPLAYDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include "../core/ProjectModel.h"  // Para MinutiaLabelPosition

namespace FingerprintEnhancer {

enum class MinutiaeSymbol {
    CIRCLE,              // Círculo simples
    CIRCLE_X,            // Círculo com X
    CIRCLE_ARROW,        // Círculo com seta (ângulo)
    CIRCLE_CROSS,        // Círculo com cruz
    TRIANGLE,            // Triângulo
    SQUARE,              // Quadrado
    DIAMOND              // Losango
};

struct MinutiaeDisplaySettings {
    MinutiaeSymbol symbol = MinutiaeSymbol::CIRCLE;
    int markerSize = 20;              // Tamanho das marcações (pixels)
    int labelFontSize = 10;           // Tamanho da fonte dos rótulos
    int lineWidth = 2;                // Largura da linha das marcações (pixels)
    QColor markerColor = QColor(255, 0, 0);  // Cor da marcação (vermelho padrão)
    QColor textColor = QColor(255, 0, 0);    // Cor do texto (vermelho padrão)
    QColor labelBackgroundColor = QColor(255, 255, 255, 200);  // Cor de fundo dos rótulos (com alpha)
    int labelBackgroundOpacity = 200; // Opacidade (0-255)
    FingerprintEnhancer::MinutiaLabelPosition defaultLabelPosition = FingerprintEnhancer::MinutiaLabelPosition::RIGHT;  // Posição padrão dos rótulos
    bool showLabelType = true;        // Mostrar tipo no rótulo (se false, mostra só número)
    bool showAngles = false;          // Mostrar ângulo da minúcia

    // Método para obter nome do símbolo
    QString getSymbolName() const {
        switch (symbol) {
            case MinutiaeSymbol::CIRCLE: return "Círculo Simples";
            case MinutiaeSymbol::CIRCLE_X: return "Círculo com X";
            case MinutiaeSymbol::CIRCLE_ARROW: return "Círculo com Seta";
            case MinutiaeSymbol::CIRCLE_CROSS: return "Círculo com Cruz";
            case MinutiaeSymbol::TRIANGLE: return "Triângulo";
            case MinutiaeSymbol::SQUARE: return "Quadrado";
            case MinutiaeSymbol::DIAMOND: return "Losango";
            default: return "Círculo Simples";
        }
    }
};

class MinutiaeDisplayDialog : public QDialog {
    Q_OBJECT

public:
    explicit MinutiaeDisplayDialog(const MinutiaeDisplaySettings& currentSettings, QWidget *parent = nullptr);

    MinutiaeDisplaySettings getSettings() const { return settings; }
    bool wasAccepted() const { return accepted; }

private slots:
    void onSymbolChanged(int index);
    void onMarkerSizeChanged(int value);
    void onLabelFontSizeChanged(int value);
    void onLineWidthChanged(int value);
    void onLabelPositionChanged(int index);
    void onChooseMarkerColor();
    void onChooseTextColor();
    void onChooseBackgroundColor();
    void onOpacityChanged(int value);
    void onShowLabelTypeChanged(int state);
    void onShowAnglesChanged(int state);
    void onAccepted();
    void onRejected();
    void onApplyClicked();
    void updatePreview();

private:
    void setupUI();
    void updateMarkerColorButton();
    void updateTextColorButton();
    void updateBgColorButton();
    void saveToGlobalSettings();

    MinutiaeDisplaySettings settings;
    bool accepted;

    // UI Components
    QComboBox* symbolCombo;
    QSpinBox* markerSizeSpinBox;
    QSpinBox* labelFontSizeSpinBox;
    QSpinBox* lineWidthSpinBox;
    QComboBox* labelPositionCombo;
    QPushButton* markerColorButton;
    QPushButton* textColorButton;
    QPushButton* bgColorButton;
    QSlider* opacitySlider;
    QLabel* opacityLabel;
    QCheckBox* showLabelTypeCheckBox;
    QCheckBox* showAnglesCheckBox;
    QLabel* previewLabel;
    QDialogButtonBox* buttonBox;
};

} // namespace FingerprintEnhancer

#endif // MINUTIAEDISPLAYDIALOG_H
