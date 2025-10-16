#include "RotationDialog.h"
#include "ImageViewer.h"
#include "MinutiaeOverlay.h"
#include "FragmentRegionsOverlay.h"
#include "../core/ProjectModel.h"
#include <QGroupBox>

RotationDialog::RotationDialog(const cv::Mat &image, ImageViewer *viewer, 
                               FingerprintEnhancer::Fragment *fragment,
                               FingerprintEnhancer::MinutiaeOverlay *overlay,
                               FingerprintEnhancer::FingerprintImage *parentImg,
                               FragmentRegionsOverlay *fragmentOverlay,
                               QWidget *parent)
    : QDialog(parent), 
      originalImage(image.clone()), 
      imageViewer(viewer),
      currentFragment(fragment),
      minutiaeOverlay(overlay),
      parentImage(parentImg),
      fragmentRegionsOverlay(fragmentOverlay),
      finalAngle(0.0), 
      accepted(false), 
      useTransparentBackground(false) {

    fprintf(stderr, "[ROTATION] Dialog criado - tem fragmentOverlay: %s\n", 
            fragmentOverlay ? "SIM" : "NAO");

    setWindowTitle("Rotacionar Imagem - Interativo");
    setMinimumSize(400, 250);
    
    // Tornar não-modal para permitir interação com viewer (zoom/scroll)
    setModal(false);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    
    // Se há fragmento, guardar minúcias originais para restaurar no cancelamento
    if (currentFragment && !currentFragment->minutiae.isEmpty()) {
        originalMinutiae = currentFragment->minutiae;
    }

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
    
    // VERIFICAR: Se rotacionando IMAGEM e o ângulo acumulado é ~0°, usar imagem original
    if (parentImage) {
        double accumulatedAngle = fmod(parentImage->currentRotationAngle + angle, 360.0);
        if (accumulatedAngle < 0) accumulatedAngle += 360.0;
        
        fprintf(stderr, "[ROTATION] Rotacionando para %.1f graus (ângulo acumulado: %.1f)\n", angle, accumulatedAngle);
        
        // Se ângulo acumulado é ~0°, usar imagem REALMENTE original (sem borda)
        if (fabs(accumulatedAngle) < 0.1 || fabs(accumulatedAngle - 360.0) < 0.1) {
            fprintf(stderr, "[ROTATION] Ângulo acumulado ~0° - usando imagem REALMENTE ORIGINAL (sem borda)\n");
            rotatedImage = parentImage->originalImage.clone();  // ✅ Usar originalImage da entidade, não do dialog
        } else {
            rotatedImage = rotateImage(originalImage, angle);
        }
    } else {
        // Rotacionando fragmento: usar delta diretamente
        fprintf(stderr, "[ROTATION] Rotacionando para %.1f graus\n", angle);
        rotatedImage = rotateImage(originalImage, angle);
    }

    // Atualizar viewer em tempo real
    if (imageViewer) {
        imageViewer->setImage(rotatedImage);
        // Ajustar zoom para evitar distorção ao mudar tamanho da imagem
        imageViewer->zoomToFit();
    }
    
    // Atualizar overlay de regiões de fragmentos se rotacionando IMAGEM
    if (parentImage && fragmentRegionsOverlay) {
        // Definir ângulo de preview (acumulado com ângulo atual da imagem)
        // IMPORTANTE: Normalizar para [0, 360) para evitar ângulos negativos
        double previewAngle = fmod(parentImage->currentRotationAngle + angle, 360.0);
        if (previewAngle < 0) previewAngle += 360.0;  // Garantir positivo
        
        fprintf(stderr, "[ROTATION] Atualizando overlay: currentAngle=%.1f + deltaAngle=%.1f = previewAngle=%.1f\n", 
                parentImage->currentRotationAngle, angle, previewAngle);
        fragmentRegionsOverlay->setPreviewRotationAngle(previewAngle);
    }
    
    // Rotacionar minúcias em tempo real se houver fragmento
    if (currentFragment && !originalMinutiae.isEmpty()) {
        // Restaurar minúcias originais primeiro
        currentFragment->minutiae = originalMinutiae;
        
        // Aplicar rotação temporária
        cv::Size oldSize = originalImage.size();
        cv::Size newSize = rotatedImage.size();
        currentFragment->rotateMinutiae(angle, oldSize, newSize);
        
        // Atualizar overlay visual
        if (minutiaeOverlay) {
            minutiaeOverlay->update();
        }
    }
}

cv::Mat RotationDialog::rotateImage(const cv::Mat &image, double angle) {
    if (image.empty()) return image;

    // ============================================================================================
    // TODO: calcular posições dos fragmentos - ROTAÇÃO DA IMAGEM (REFERÊNCIA)
    // ============================================================================================
    // Esta é a função que REALMENTE rotaciona a imagem usando OpenCV.
    // O overlay de fragmentos (FragmentRegionsOverlay::calculateRotatedPolygon) tenta replicar
    // EXATAMENTE esta transformação para calcular onde os retângulos aparecem.
    //
    // ENTRADA:
    // - image : cv::Mat da imagem a rotacionar (originalImage da entidade)
    // - angle : Ângulo em graus (delta, não acumulado)
    //
    // SAÍDA:
    // - cv::Mat rotacionado (pode ter tamanho maior com borda branca para ângulos arbitrários)
    //
    // MATRIZ DE ROTAÇÃO USADA:
    // - cv::getRotationMatrix2D(center, -angle, 1.0)
    // - Ajuste de offset: rotMatrix.at<double>(0,2) += (new_w/2.0) - center.x
    //                     rotMatrix.at<double>(1,2) += (new_h/2.0) - center.y
    // - Aplicada com: cv::warpAffine(image, rotated, rotMatrix, cv::Size(new_w, new_h))
    // ============================================================================================

    // Se o ângulo é ~0°, retornar imagem original sem borda branca
    double normalizedAngle = fmod(angle, 360.0);
    if (normalizedAngle < 0) normalizedAngle += 360.0;
    
    if (fabs(normalizedAngle) < 0.1 || fabs(normalizedAngle - 360.0) < 0.1) {
        fprintf(stderr, "[ROTATION] rotateImage: %.1f graus (~0°) - retornando imagem ORIGINAL (sem borda)\n", angle);
        return image.clone();
    }
    
    // Para múltiplos exatos de 90°, usar rotate() em vez de warpAffine (sem borda branca)
    bool isMultipleOf90 = false;
    int rotateCode = -1;
    
    if (fabs(normalizedAngle - 90.0) < 0.1) {
        isMultipleOf90 = true;
        rotateCode = cv::ROTATE_90_COUNTERCLOCKWISE;
        fprintf(stderr, "[ROTATION] rotateImage: %.1f graus (~90°) - usando cv::rotate (sem borda)\n", angle);
    } else if (fabs(normalizedAngle - 180.0) < 0.1) {
        isMultipleOf90 = true;
        rotateCode = cv::ROTATE_180;
        fprintf(stderr, "[ROTATION] rotateImage: %.1f graus (~180°) - usando cv::rotate (sem borda)\n", angle);
    } else if (fabs(normalizedAngle - 270.0) < 0.1) {
        isMultipleOf90 = true;
        rotateCode = cv::ROTATE_90_CLOCKWISE;
        fprintf(stderr, "[ROTATION] rotateImage: %.1f graus (~270°) - usando cv::rotate (sem borda)\n", angle);
    }
    
    if (isMultipleOf90) {
        cv::Mat rotated;
        cv::rotate(image, rotated, rotateCode);
        fprintf(stderr, "[ROTATION] Imagem rotacionada: canais=%d, tamanho=%dx%d\n",
                rotated.channels(), rotated.cols, rotated.rows);
        return rotated;
    }

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
    
    fprintf(stderr, "[ROTATION] rotateImage: %.1f graus, canais=%d, transparente=%s\n", 
            angle, image.channels(), useTransparentBackground ? "SIM" : "NAO");

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
        // Rotacionar com fundo branco
        // IMPORTANTE: Usar Scalar apropriado para número de canais
        cv::Scalar borderColor;
        if (image.channels() == 1) {
            borderColor = cv::Scalar(255);  // Grayscale
        } else if (image.channels() == 3) {
            borderColor = cv::Scalar(255, 255, 255);  // BGR
        } else if (image.channels() == 4) {
            borderColor = cv::Scalar(255, 255, 255, 255);  // BGRA (PNG com alpha)
        } else {
            borderColor = cv::Scalar(255);
        }
        
        cv::warpAffine(image, rotated, rotMatrix, cv::Size(new_w, new_h),
                       cv::INTER_LINEAR, cv::BORDER_CONSTANT, borderColor);
    }
    
    fprintf(stderr, "[ROTATION] Imagem rotacionada: canais=%d, tamanho=%dx%d\n",
            rotated.channels(), rotated.cols, rotated.rows);

    return rotated;
}

void RotationDialog::onAccept() {
    accepted = true;
    
    // Limpar preview de rotação do overlay de fragmentos
    if (fragmentRegionsOverlay) {
        fprintf(stderr, "[ROTATION] Limpando preview de rotação do overlay ao aceitar\n");
        fragmentRegionsOverlay->clearPreviewRotationAngle();
    }
    
    accept();
}

void RotationDialog::onReject() {
    accepted = false;
    
    // Restaurar imagem original no viewer
    if (imageViewer) {
        imageViewer->setImage(originalImage);
    }
    
    // Limpar preview de rotação do overlay de fragmentos
    if (fragmentRegionsOverlay) {
        fprintf(stderr, "[ROTATION] Limpando preview de rotação do overlay\n");
        fragmentRegionsOverlay->clearPreviewRotationAngle();
    }
    
    // Restaurar minúcias originais se houver fragmento
    if (currentFragment && !originalMinutiae.isEmpty()) {
        currentFragment->minutiae = originalMinutiae;
        
        // Atualizar overlay visual
        if (minutiaeOverlay) {
            minutiaeOverlay->update();
        }
    }
    
    reject();
}
