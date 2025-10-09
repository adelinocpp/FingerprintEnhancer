#include "ColorConversionDialog.h"
#include "ImageViewer.h"
#include <QSplitter>

ColorConversionDialog::ColorConversionDialog(const cv::Mat &image, ColorSpaceType targetSpace, QWidget *parent)
    : QDialog(parent),
      originalImage(image.clone()),
      targetColorSpace(targetSpace),
      accepted(false),
      channel1Adjust(0),
      channel2Adjust(0),
      channel3Adjust(0) {

    setWindowTitle("Conversão para " + getColorSpaceName());
    setMinimumSize(900, 600);

    setupUI();

    // Realizar conversão inicial
    convertToTargetSpace();
    updatePreview();
}

ColorConversionDialog::~ColorConversionDialog() = default;

void ColorConversionDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Splitter para preview
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // Preview
    QGroupBox *previewGroup = new QGroupBox("Preview (espaço " + getColorSpaceName() + ")");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

    previewViewer = new ImageViewer();
    previewLayout->addWidget(previewViewer);

    splitter->addWidget(previewGroup);

    // Controles
    QGroupBox *controlGroup = new QGroupBox("Ajuste de Canais");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);

    QString ch1Name, ch2Name, ch3Name;
    getChannelNames(ch1Name, ch2Name, ch3Name);

    // Canal 1
    QGroupBox *channel1Group = new QGroupBox("Canal " + ch1Name);
    QVBoxLayout *ch1Layout = new QVBoxLayout(channel1Group);

    QHBoxLayout *ch1SliderLayout = new QHBoxLayout();
    ch1SliderLayout->addWidget(new QLabel("-255"));
    channel1Slider = new QSlider(Qt::Horizontal);
    channel1Slider->setRange(-255, 255);
    channel1Slider->setValue(0);
    channel1Slider->setTickPosition(QSlider::TicksBelow);
    channel1Slider->setTickInterval(50);
    ch1SliderLayout->addWidget(channel1Slider);
    ch1SliderLayout->addWidget(new QLabel("+255"));
    ch1Layout->addLayout(ch1SliderLayout);

    QHBoxLayout *ch1SpinLayout = new QHBoxLayout();
    ch1SpinLayout->addWidget(new QLabel("Ajuste:"));
    channel1SpinBox = new QSpinBox();
    channel1SpinBox->setRange(-255, 255);
    channel1SpinBox->setValue(0);
    ch1SpinLayout->addWidget(channel1SpinBox);
    ch1SpinLayout->addStretch();
    ch1Layout->addLayout(ch1SpinLayout);

    controlLayout->addWidget(channel1Group);

    // Canal 2
    QGroupBox *channel2Group = new QGroupBox("Canal " + ch2Name);
    QVBoxLayout *ch2Layout = new QVBoxLayout(channel2Group);

    QHBoxLayout *ch2SliderLayout = new QHBoxLayout();
    ch2SliderLayout->addWidget(new QLabel("-255"));
    channel2Slider = new QSlider(Qt::Horizontal);
    channel2Slider->setRange(-255, 255);
    channel2Slider->setValue(0);
    channel2Slider->setTickPosition(QSlider::TicksBelow);
    channel2Slider->setTickInterval(50);
    ch2SliderLayout->addWidget(channel2Slider);
    ch2SliderLayout->addWidget(new QLabel("+255"));
    ch2Layout->addLayout(ch2SliderLayout);

    QHBoxLayout *ch2SpinLayout = new QHBoxLayout();
    ch2SpinLayout->addWidget(new QLabel("Ajuste:"));
    channel2SpinBox = new QSpinBox();
    channel2SpinBox->setRange(-255, 255);
    channel2SpinBox->setValue(0);
    ch2SpinLayout->addWidget(channel2SpinBox);
    ch2SpinLayout->addStretch();
    ch2Layout->addLayout(ch2SpinLayout);

    controlLayout->addWidget(channel2Group);

    // Canal 3
    QGroupBox *channel3Group = new QGroupBox("Canal " + ch3Name);
    QVBoxLayout *ch3Layout = new QVBoxLayout(channel3Group);

    QHBoxLayout *ch3SliderLayout = new QHBoxLayout();
    ch3SliderLayout->addWidget(new QLabel("-255"));
    channel3Slider = new QSlider(Qt::Horizontal);
    channel3Slider->setRange(-255, 255);
    channel3Slider->setValue(0);
    channel3Slider->setTickPosition(QSlider::TicksBelow);
    channel3Slider->setTickInterval(50);
    ch3SliderLayout->addWidget(channel3Slider);
    ch3SliderLayout->addWidget(new QLabel("+255"));
    ch3Layout->addLayout(ch3SliderLayout);

    QHBoxLayout *ch3SpinLayout = new QHBoxLayout();
    ch3SpinLayout->addWidget(new QLabel("Ajuste:"));
    channel3SpinBox = new QSpinBox();
    channel3SpinBox->setRange(-255, 255);
    channel3SpinBox->setValue(0);
    ch3SpinLayout->addWidget(channel3SpinBox);
    ch3SpinLayout->addStretch();
    ch3Layout->addLayout(ch3SpinLayout);

    controlLayout->addWidget(channel3Group);

    // Botão reset
    resetButton = new QPushButton("Resetar Ajustes");
    controlLayout->addWidget(resetButton);

    controlLayout->addStretch();

    splitter->addWidget(controlGroup);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // Botões de ação
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QLabel *infoLabel = new QLabel("Ajuste os canais e visualize em tempo real. Clique 'Aplicar' para confirmar.");
    infoLabel->setStyleSheet("color: gray; font-style: italic;");
    buttonLayout->addWidget(infoLabel);

    buttonLayout->addStretch();

    cancelButton = new QPushButton("Cancelar");
    acceptButton = new QPushButton("Aplicar");
    acceptButton->setDefault(true);

    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(acceptButton);

    mainLayout->addLayout(buttonLayout);

    // Conectar sinais
    connect(channel1Slider, &QSlider::valueChanged, this, &ColorConversionDialog::onChannel1Changed);
    connect(channel1SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColorConversionDialog::onChannel1Changed);

    connect(channel2Slider, &QSlider::valueChanged, this, &ColorConversionDialog::onChannel2Changed);
    connect(channel2SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColorConversionDialog::onChannel2Changed);

    connect(channel3Slider, &QSlider::valueChanged, this, &ColorConversionDialog::onChannel3Changed);
    connect(channel3SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColorConversionDialog::onChannel3Changed);

    connect(resetButton, &QPushButton::clicked, this, &ColorConversionDialog::onResetChannels);
    connect(acceptButton, &QPushButton::clicked, this, &ColorConversionDialog::onAccept);
    connect(cancelButton, &QPushButton::clicked, this, &ColorConversionDialog::onReject);
}

void ColorConversionDialog::convertToTargetSpace() {
    cv::Mat temp = originalImage.clone();

    // Garantir que está em BGR primeiro
    if (temp.channels() == 1) {
        cv::cvtColor(temp, temp, cv::COLOR_GRAY2BGR);
    } else if (temp.channels() == 4) {
        cv::cvtColor(temp, temp, cv::COLOR_BGRA2BGR);
    }

    // Converter para espaço de destino
    switch (targetColorSpace) {
        case COLOR_SPACE_RGB: {
            // OpenCV usa BGR internamente, então não precisa conversão
            workingImage = temp.clone();
            break;
        }

        case COLOR_SPACE_HSV: {
            cv::cvtColor(temp, workingImage, cv::COLOR_BGR2HSV);
            break;
        }

        case COLOR_SPACE_HSI: {
            // Aproximação HSI usando HSV como base
            cv::cvtColor(temp, workingImage, cv::COLOR_BGR2HSV);

            // Calcular Intensity como média dos canais BGR
            std::vector<cv::Mat> bgrChannels;
            cv::split(temp, bgrChannels);
            cv::Mat intensity = (bgrChannels[0] + bgrChannels[1] + bgrChannels[2]) / 3;

            // Substituir canal V por I
            std::vector<cv::Mat> hsiChannels;
            cv::split(workingImage, hsiChannels);
            hsiChannels[2] = intensity;
            cv::merge(hsiChannels, workingImage);
            break;
        }

        case COLOR_SPACE_LAB: {
            cv::cvtColor(temp, workingImage, cv::COLOR_BGR2Lab);
            break;
        }
    }

    convertedImage = workingImage.clone();
}

void ColorConversionDialog::onChannel1Changed(int value) {
    // Sincronizar slider e spinbox
    channel1Slider->blockSignals(true);
    channel1SpinBox->blockSignals(true);
    channel1Slider->setValue(value);
    channel1SpinBox->setValue(value);
    channel1Slider->blockSignals(false);
    channel1SpinBox->blockSignals(false);

    channel1Adjust = value;
    updatePreview();
}

void ColorConversionDialog::onChannel2Changed(int value) {
    channel2Slider->blockSignals(true);
    channel2SpinBox->blockSignals(true);
    channel2Slider->setValue(value);
    channel2SpinBox->setValue(value);
    channel2Slider->blockSignals(false);
    channel2SpinBox->blockSignals(false);

    channel2Adjust = value;
    updatePreview();
}

void ColorConversionDialog::onChannel3Changed(int value) {
    channel3Slider->blockSignals(true);
    channel3SpinBox->blockSignals(true);
    channel3Slider->setValue(value);
    channel3SpinBox->setValue(value);
    channel3Slider->blockSignals(false);
    channel3SpinBox->blockSignals(false);

    channel3Adjust = value;
    updatePreview();
}

void ColorConversionDialog::onResetChannels() {
    channel1Slider->setValue(0);
    channel2Slider->setValue(0);
    channel3Slider->setValue(0);

    channel1Adjust = 0;
    channel2Adjust = 0;
    channel3Adjust = 0;

    updatePreview();
}

void ColorConversionDialog::updatePreview() {
    convertedImage = workingImage.clone();

    // Aplicar ajustes nos canais
    if (channel1Adjust != 0 || channel2Adjust != 0 || channel3Adjust != 0) {
        applyChannelAdjustments();
    }

    // Converter de volta para BGR para visualização
    cv::Mat display;
    switch (targetColorSpace) {
        case COLOR_SPACE_RGB:
            display = convertedImage.clone();
            break;

        case COLOR_SPACE_HSV:
            cv::cvtColor(convertedImage, display, cv::COLOR_HSV2BGR);
            break;

        case COLOR_SPACE_HSI:
            // Converter HSI de volta (aproximação via HSV)
            cv::cvtColor(convertedImage, display, cv::COLOR_HSV2BGR);
            break;

        case COLOR_SPACE_LAB:
            cv::cvtColor(convertedImage, display, cv::COLOR_Lab2BGR);
            break;
    }

    previewViewer->setImage(display);
}

void ColorConversionDialog::applyChannelAdjustments() {
    std::vector<cv::Mat> channels;
    cv::split(convertedImage, channels);

    // Aplicar ajustes com saturação
    for (int i = 0; i < convertedImage.rows; i++) {
        for (int j = 0; j < convertedImage.cols; j++) {
            if (channel1Adjust != 0) {
                int val = channels[0].at<uchar>(i, j) + channel1Adjust;
                channels[0].at<uchar>(i, j) = cv::saturate_cast<uchar>(val);
            }
            if (channel2Adjust != 0) {
                int val = channels[1].at<uchar>(i, j) + channel2Adjust;
                channels[1].at<uchar>(i, j) = cv::saturate_cast<uchar>(val);
            }
            if (channel3Adjust != 0) {
                int val = channels[2].at<uchar>(i, j) + channel3Adjust;
                channels[2].at<uchar>(i, j) = cv::saturate_cast<uchar>(val);
            }
        }
    }

    cv::merge(channels, convertedImage);
}

QString ColorConversionDialog::getColorSpaceName() const {
    switch (targetColorSpace) {
        case COLOR_SPACE_RGB: return "RGB";
        case COLOR_SPACE_HSV: return "HSV";
        case COLOR_SPACE_HSI: return "HSI";
        case COLOR_SPACE_LAB: return "Lab";
        default: return "Desconhecido";
    }
}

void ColorConversionDialog::getChannelNames(QString &ch1, QString &ch2, QString &ch3) const {
    switch (targetColorSpace) {
        case COLOR_SPACE_RGB:
            ch1 = "B (Blue)";
            ch2 = "G (Green)";
            ch3 = "R (Red)";
            break;
        case COLOR_SPACE_HSV:
            ch1 = "H (Hue)";
            ch2 = "S (Saturation)";
            ch3 = "V (Value)";
            break;
        case COLOR_SPACE_HSI:
            ch1 = "H (Hue)";
            ch2 = "S (Saturation)";
            ch3 = "I (Intensity)";
            break;
        case COLOR_SPACE_LAB:
            ch1 = "L (Lightness)";
            ch2 = "a (green-red)";
            ch3 = "b (blue-yellow)";
            break;
    }
}

void ColorConversionDialog::onAccept() {
    accepted = true;
    accept();
}

void ColorConversionDialog::onReject() {
    accepted = false;
    reject();
}
