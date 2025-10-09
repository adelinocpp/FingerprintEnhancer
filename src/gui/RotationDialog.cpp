#include "RotationDialog.h"
#include "ImageViewer.h"
#include <QGroupBox>

RotationDialog::RotationDialog(const cv::Mat &image, ImageViewer *viewer, QWidget *parent)
    : QDialog(parent), originalImage(image.clone()), imageViewer(viewer),
      finalAngle(0.0), accepted(false), useTransparentBackground(false) {

    setWindowTitle("Rotacionar Imagem");
    setMinimumSize(400, 250);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grupo de controles
    QGroupBox *controlGroup = new QGroupBox("Ângulo de Rotação");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);

    // Slider (-180 a +180 graus)
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    sliderLayout->addWidget(new QLabel("-180°"));
    angleSlider = new QSlider(Qt::Horizontal);
    angleSlider->setRange(-1800, 1800); // -180.0 a 180.0 (multiplicado por 10)
    angleSlider->setValue(0);
    angleSlider->setTickPosition(QSlider::TicksBelow);
    angleSlider->setTickInterval(450); // Tick a cada 45°
    sliderLayout->addWidget(angleSlider);
    sliderLayout->addWidget(new QLabel("+180°"));
    controlLayout->addLayout(sliderLayout);

    // SpinBox para valor preciso
    QHBoxLayout *spinLayout = new QHBoxLayout();
    spinLayout->addWidget(new QLabel("Ângulo exato:"));
    angleSpinBox = new QDoubleSpinBox();
    angleSpinBox->setRange(-180.0, 180.0);
    angleSpinBox->setValue(0.0);
    angleSpinBox->setDecimals(1);
    angleSpinBox->setSingleStep(0.5);
    angleSpinBox->setSuffix("°");
    spinLayout->addWidget(angleSpinBox);
    spinLayout->addStretch();
    controlLayout->addLayout(spinLayout);

    QLabel *helpLabel = new QLabel("(Positivo = horário, Negativo = anti-horário)");
    helpLabel->setStyleSheet("color: gray; font-size: 10pt;");
    controlLayout->addWidget(helpLabel);

    mainLayout->addWidget(controlGroup);

    // Opção de fundo transparente
    transparentBgCheckbox = new QCheckBox("Usar fundo transparente (para filtros FFT)");
    transparentBgCheckbox->setToolTip("Preenche o fundo com transparência em vez de branco.\n"
                                       "Útil para aplicar filtros FFT posteriormente.");
    mainLayout->addWidget(transparentBgCheckbox);

    // Botões
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    cancelButton = new QPushButton("Cancelar");
    acceptButton = new QPushButton("Aplicar");
    acceptButton->setDefault(true);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(acceptButton);
    mainLayout->addLayout(buttonLayout);

    // Conectar sinais
    connect(angleSlider, &QSlider::valueChanged, this, &RotationDialog::onSliderChanged);
    connect(angleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RotationDialog::onSpinBoxChanged);
    connect(transparentBgCheckbox, &QCheckBox::stateChanged, this, &RotationDialog::onTransparentBgChanged);
    connect(acceptButton, &QPushButton::clicked, this, &RotationDialog::onAccept);
    connect(cancelButton, &QPushButton::clicked, this, &RotationDialog::onReject);

    // Preview inicial
    updatePreview(0.0);
}

RotationDialog::~RotationDialog() = default;

void RotationDialog::onSliderChanged(int value) {
    double angle = value / 10.0;
    angleSpinBox->blockSignals(true);
    angleSpinBox->setValue(angle);
    angleSpinBox->blockSignals(false);
    updatePreview(angle);
}

void RotationDialog::onSpinBoxChanged(double value) {
    int sliderValue = static_cast<int>(value * 10);
    angleSlider->blockSignals(true);
    angleSlider->setValue(sliderValue);
    angleSlider->blockSignals(false);
    updatePreview(value);
}

void RotationDialog::onTransparentBgChanged(int state) {
    useTransparentBackground = (state == Qt::Checked);
    // Atualizar preview com nova configuração
    updatePreview(finalAngle);
}

void RotationDialog::updatePreview(double angle) {
    finalAngle = angle;
    rotatedImage = rotateImage(originalImage, angle);

    // Atualizar viewer em tempo real
    if (imageViewer) {
        imageViewer->setImage(rotatedImage);
    }
}

cv::Mat RotationDialog::rotateImage(const cv::Mat &image, double angle) {
    if (image.empty()) return image;

    // Converter ângulo para radianos (OpenCV usa sentido anti-horário, então invertemos)
    cv::Point2f center(image.cols / 2.0f, image.rows / 2.0f);
    cv::Mat rotMatrix = cv::getRotationMatrix2D(center, -angle, 1.0);

    // Calcular novo tamanho da imagem para não cortar
    double abs_cos = abs(rotMatrix.at<double>(0, 0));
    double abs_sin = abs(rotMatrix.at<double>(0, 1));
    int new_w = int(image.rows * abs_sin + image.cols * abs_cos);
    int new_h = int(image.rows * abs_cos + image.cols * abs_sin);

    // Ajustar matriz de rotação para o novo tamanho
    rotMatrix.at<double>(0, 2) += (new_w / 2.0) - center.x;
    rotMatrix.at<double>(1, 2) += (new_h / 2.0) - center.y;

    cv::Mat rotated;

    if (useTransparentBackground) {
        // Converter para BGRA se necessário
        cv::Mat imageWithAlpha;
        if (image.channels() == 3) {
            cv::cvtColor(image, imageWithAlpha, cv::COLOR_BGR2BGRA);
        } else if (image.channels() == 1) {
            cv::cvtColor(image, imageWithAlpha, cv::COLOR_GRAY2BGRA);
        } else if (image.channels() == 4) {
            imageWithAlpha = image.clone();
        } else {
            imageWithAlpha = image.clone();
        }

        // Rotacionar com fundo transparente
        cv::warpAffine(imageWithAlpha, rotated, rotMatrix, cv::Size(new_w, new_h),
                       cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));
    } else {
        // Rotacionar com fundo branco (comportamento padrão)
        cv::warpAffine(image, rotated, rotMatrix, cv::Size(new_w, new_h),
                       cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
    }

    return rotated;
}

void RotationDialog::onAccept() {
    accepted = true;
    accept();
}

void RotationDialog::onReject() {
    accepted = false;
    // Restaurar imagem original no viewer
    if (imageViewer) {
        imageViewer->setImage(originalImage);
    }
    reject();
}
