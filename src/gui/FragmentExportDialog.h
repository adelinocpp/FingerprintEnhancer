#ifndef FRAGMENTEXPORTDIALOG_H
#define FRAGMENTEXPORTDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSlider>
#include <QGroupBox>
#include <opencv2/opencv.hpp>
#include "../core/ProjectModel.h"
#include "MinutiaeDisplayDialog.h"

/**
 * @brief Dialog para exportação de fragmentos com opções avançadas
 * 
 * Permite configurar:
 * - Resolução de saída
 * - Tamanho das marcações e fontes
 * - Opções de visualização (cores, estilos)
 * - Formato e qualidade
 * - Preview em tempo real
 */
class FragmentExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit FragmentExportDialog(
        const FingerprintEnhancer::Fragment* fragment,
        double currentScale,
        QWidget *parent = nullptr
    );
    ~FragmentExportDialog();

    // Getters para configurações
    QString getFilePath() const { return filePathEdit->text(); }
    bool includeMinutiae() const { return includeMinutiaeCheck->isChecked(); }
    int getMarkerSize() const { return markerSizeSpinBox->value(); }
    int getFontSize() const { return fontSizeSpinBox->value(); }
    int getOutputWidth() const { return widthSpinBox->value(); }
    int getOutputHeight() const { return heightSpinBox->value(); }
    int getDPI() const { return dpiSpinBox->value(); }
    int getQuality() const { return qualitySlider->value(); }
    QString getFormat() const;
    
    // Configurações de visualização
    bool showMinutiaeNumbers() const { return showNumbersCheck->isChecked(); }
    bool showMinutiaeTypes() const { return showLabelTypeCheck->isChecked(); }
    bool showAngles() const { return showAnglesCheck->isChecked(); }
    QColor getMarkerColor() const { return markerColor; }
    QColor getTextColor() const { return textColor; }
    QColor getLabelBgColor() const { return labelBgColor; }
    
    // Exportação
    bool exportImage(QString& errorMessage);

private slots:
    void onBrowseClicked();
    void onFormatChanged(int index);
    void onAnySettingChanged();
    void onWidthChanged(int value);
    void onHeightChanged(int value);
    void onScalePercentChanged(int value);
    void updatePreview();
    void onMarkerColorClicked();
    void onTextColorClicked();
    void onResetToDefaults();

private:
    void setupUI();
    void loadDefaultSettings();
    cv::Mat renderPreview();
    cv::Mat renderExportImage(int width, int height);
    void drawMinutiaSymbol(cv::Mat& img, const cv::Point& center, int radius, 
                          float angle, const cv::Scalar& color);
    void drawMinutiaLabels(cv::Mat& img, const cv::Point& center,
                          const QString& numberLabel, const QString& typeLabel,
                          FingerprintEnhancer::MinutiaLabelPosition labelPos,
                          int markerRadius, double fontSize, int fontThickness,
                          const cv::Scalar& textColor, const cv::Scalar& bgColor,
                          double scaleFactor);
    QPixmap cvMatToQPixmap(const cv::Mat& mat);
    QString estimateFileSize(int width, int height);

    // Dados
    const FingerprintEnhancer::Fragment* fragment;
    double scale;
    
    // Widgets - Arquivo
    QLineEdit *filePathEdit;
    QPushButton *browseButton;
    QComboBox *formatComboBox;
    QSlider *qualitySlider;
    QLabel *qualityLabel;
    
    // Widgets - Resolução
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    QSpinBox *scalePercentSpinBox;
    QSpinBox *dpiSpinBox;
    QCheckBox *maintainAspectCheck;
    QPushButton *resetResolutionButton;
    
    // Widgets - Marcações
    QCheckBox *includeMinutiaeCheck;
    QSpinBox *markerSizeSpinBox;
    QSpinBox *fontSizeSpinBox;
    QComboBox *symbolComboBox;
    QComboBox *labelPositionComboBox;
    QCheckBox *showNumbersCheck;
    QCheckBox *showLabelTypeCheck;
    QCheckBox *showAnglesCheck;
    QSlider *labelOpacitySlider;
    QLabel *labelOpacityLabel;
    QPushButton *labelBgColorButton;
    QColor labelBgColor;
    
    // Widgets - Cores
    QPushButton *markerColorButton;
    QPushButton *textColorButton;
    QColor markerColor;
    QColor textColor;
    
    // Widgets - Preview
    QLabel *previewLabel;
    QLabel *infoLabel;
    
    // Botões
    QPushButton *exportButton;
    QPushButton *cancelButton;
    QPushButton *resetButton;
    
    // Dimensões originais
    int originalWidth;
    int originalHeight;
    bool updatingResolution;
    
    // Configurações de visualização
    FingerprintEnhancer::MinutiaeDisplaySettings displaySettings;
};

#endif // FRAGMENTEXPORTDIALOG_H
