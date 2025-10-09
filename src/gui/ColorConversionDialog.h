#ifndef COLORCONVERSIONDIALOG_H
#define COLORCONVERSIONDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <opencv2/opencv.hpp>

class ImageViewer;

/**
 * @brief Tipos de conversão de espaço de cor suportados
 */
enum ColorSpaceType {
    COLOR_SPACE_RGB,
    COLOR_SPACE_HSV,
    COLOR_SPACE_HSI,
    COLOR_SPACE_LAB
};

/**
 * @brief Diálogo interativo para conversão e ajuste de espaços de cor
 *
 * Permite converter entre espaços de cor com preview em tempo real
 * e ajuste individual de canais.
 */
class ColorConversionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColorConversionDialog(const cv::Mat &image, ColorSpaceType targetSpace, QWidget *parent = nullptr);
    ~ColorConversionDialog();

    cv::Mat getConvertedImage() const { return convertedImage; }
    bool wasAccepted() const { return accepted; }

private slots:
    void onChannel1Changed(int value);
    void onChannel2Changed(int value);
    void onChannel3Changed(int value);
    void onResetChannels();
    void onAccept();
    void onReject();

private:
    // Imagens
    cv::Mat originalImage;
    cv::Mat convertedImage;
    cv::Mat workingImage;  // Imagem no espaço de cor de trabalho

    // Estado
    ColorSpaceType targetColorSpace;
    bool accepted;

    // Ajustes de canais (offset -255 a +255)
    int channel1Adjust;
    int channel2Adjust;
    int channel3Adjust;

    // Widgets
    ImageViewer *previewViewer;

    // Controles de canal 1
    QLabel *channel1Label;
    QSlider *channel1Slider;
    QSpinBox *channel1SpinBox;

    // Controles de canal 2
    QLabel *channel2Label;
    QSlider *channel2Slider;
    QSpinBox *channel2SpinBox;

    // Controles de canal 3
    QLabel *channel3Label;
    QSlider *channel3Slider;
    QSpinBox *channel3SpinBox;

    QPushButton *resetButton;
    QPushButton *acceptButton;
    QPushButton *cancelButton;

    // Métodos auxiliares
    void setupUI();
    void convertToTargetSpace();
    void updatePreview();
    void applyChannelAdjustments();
    QString getColorSpaceName() const;
    void getChannelNames(QString &ch1, QString &ch2, QString &ch3) const;
};

#endif // COLORCONVERSIONDIALOG_H
