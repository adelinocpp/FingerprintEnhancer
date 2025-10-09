#include "FFTFilterDialog.h"
#include "ImageViewer.h"
#include <QGroupBox>
#include <QSplitter>
#include <QPainter>
#include <QDebug>

// ==================== FFTSpectrumLabel ====================

FFTSpectrumLabel::FFTSpectrumLabel(QWidget *parent)
    : QLabel(parent), isDrawing(false), invertMask(false) {
    setMouseTracking(true);
    setMinimumSize(400, 400);
    setScaledContents(false);
    setAlignment(Qt::AlignCenter);
    setStyleSheet("QLabel { background-color: black; border: 2px solid gray; }");
}

void FFTSpectrumLabel::setSpectrum(const cv::Mat &spectrum) {
    if (spectrum.empty()) return;

    // Converter cv::Mat para QPixmap
    cv::Mat display;
    if (spectrum.channels() == 1) {
        cv::cvtColor(spectrum, display, cv::COLOR_GRAY2RGB);
    } else {
        display = spectrum.clone();
    }

    QImage qimg(display.data, display.cols, display.rows, display.step, QImage::Format_RGB888);
    spectrumPixmap = QPixmap::fromImage(qimg.copy());

    setPixmap(spectrumPixmap);
    update();
}

void FFTSpectrumLabel::addMaskRect(const QRect &rect) {
    maskRects.append(rect);
    update();
    emit maskChanged();
}

void FFTSpectrumLabel::clearMasks() {
    maskRects.clear();
    update();
    emit maskChanged();
}

void FFTSpectrumLabel::paintEvent(QPaintEvent *event) {
    QLabel::paintEvent(event);

    if (spectrumPixmap.isNull()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Calcular offset para centralização
    int offsetX = (width() - spectrumPixmap.width()) / 2;
    int offsetY = (height() - spectrumPixmap.height()) / 2;

    // Desenhar retângulos de máscara
    painter.setPen(QPen(invertMask ? Qt::green : Qt::red, 2, Qt::SolidLine));
    painter.setBrush(QBrush(invertMask ? QColor(0, 255, 0, 50) : QColor(255, 0, 0, 50)));

    for (const QRect &rect : maskRects) {
        painter.drawRect(rect.translated(offsetX, offsetY));
    }

    // Desenhar retângulo atual sendo desenhado
    if (isDrawing && !currentRect.isNull()) {
        painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));
        painter.setBrush(QBrush(QColor(255, 255, 0, 30)));
        painter.drawRect(currentRect.translated(offsetX, offsetY));
    }
}

void FFTSpectrumLabel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !spectrumPixmap.isNull()) {
        // Calcular posição relativa ao pixmap
        int offsetX = (width() - spectrumPixmap.width()) / 2;
        int offsetY = (height() - spectrumPixmap.height()) / 2;

        QPoint relPos = event->pos() - QPoint(offsetX, offsetY);

        // Verificar se está dentro do pixmap
        if (relPos.x() >= 0 && relPos.x() < spectrumPixmap.width() &&
            relPos.y() >= 0 && relPos.y() < spectrumPixmap.height()) {
            isDrawing = true;
            dragStart = relPos;
            currentRect = QRect(dragStart, dragStart);
        }
    }
}

void FFTSpectrumLabel::mouseMoveEvent(QMouseEvent *event) {
    if (isDrawing) {
        int offsetX = (width() - spectrumPixmap.width()) / 2;
        int offsetY = (height() - spectrumPixmap.height()) / 2;

        QPoint relPos = event->pos() - QPoint(offsetX, offsetY);

        // Limitar ao tamanho do pixmap
        relPos.setX(qBound(0, relPos.x(), spectrumPixmap.width() - 1));
        relPos.setY(qBound(0, relPos.y(), spectrumPixmap.height() - 1));

        currentRect = QRect(dragStart, relPos).normalized();
        update();
    }
}

void FFTSpectrumLabel::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isDrawing) {
        isDrawing = false;

        if (!currentRect.isNull() && currentRect.width() > 5 && currentRect.height() > 5) {
            addMaskRect(currentRect);
        }

        currentRect = QRect();
        update();
    }
}

// ==================== FFTFilterDialog ====================

FFTFilterDialog::FFTFilterDialog(const cv::Mat &image, QWidget *parent)
    : QDialog(parent), originalImage(image.clone()), accepted(false), invertMask(false) {

    setWindowTitle("Filtro FFT Interativo");
    setMinimumSize(1000, 600);

    // Calcular FFT
    computeFFT();
    computeMagnitudeSpectrum();

    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Splitter para dividir espectro e preview
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // Grupo do espectro
    QGroupBox *spectrumGroup = new QGroupBox("Espectro de Frequências (clique e arraste para selecionar)");
    QVBoxLayout *spectrumLayout = new QVBoxLayout(spectrumGroup);

    spectrumLabel = new FFTSpectrumLabel();
    spectrumLabel->setSpectrum(magnitudeSpectrum);
    spectrumLayout->addWidget(spectrumLabel);

    splitter->addWidget(spectrumGroup);

    // Grupo do preview
    QGroupBox *previewGroup = new QGroupBox("Preview Filtrado (tempo real)");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

    previewViewer = new ImageViewer();
    previewViewer->setImage(originalImage);
    previewLayout->addWidget(previewViewer);

    splitter->addWidget(previewGroup);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // Controles
    QHBoxLayout *controlLayout = new QHBoxLayout();

    invertMaskCheckbox = new QCheckBox("Inverter máscara (manter selecionado, remover resto)");
    invertMaskCheckbox->setToolTip("Se marcado, mantém as frequências selecionadas e remove as outras.");
    controlLayout->addWidget(invertMaskCheckbox);

    controlLayout->addStretch();

    clearButton = new QPushButton("Limpar Máscaras");
    controlLayout->addWidget(clearButton);

    cancelButton = new QPushButton("Cancelar");
    acceptButton = new QPushButton("Aplicar");
    acceptButton->setDefault(true);

    controlLayout->addWidget(cancelButton);
    controlLayout->addWidget(acceptButton);

    mainLayout->addLayout(controlLayout);

    // Conectar sinais
    connect(spectrumLabel, &FFTSpectrumLabel::maskChanged, this, &FFTFilterDialog::onMaskChanged);
    connect(invertMaskCheckbox, &QCheckBox::stateChanged, this, &FFTFilterDialog::onInvertMaskChanged);
    connect(clearButton, &QPushButton::clicked, this, &FFTFilterDialog::onClearMasks);
    connect(acceptButton, &QPushButton::clicked, this, &FFTFilterDialog::onAccept);
    connect(cancelButton, &QPushButton::clicked, this, &FFTFilterDialog::onReject);

    // Preview inicial
    filteredImage = originalImage.clone();
}

FFTFilterDialog::~FFTFilterDialog() = default;

void FFTFilterDialog::computeFFT() {
    cv::Mat gray;
    if (originalImage.channels() > 1) {
        cv::cvtColor(originalImage, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = originalImage.clone();
    }

    // Expandir para tamanho ótimo
    int m = cv::getOptimalDFTSize(gray.rows);
    int n = cv::getOptimalDFTSize(gray.cols);
    cv::Mat padded;
    cv::copyMakeBorder(gray, padded, 0, m - gray.rows, 0, n - gray.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));

    // Preparar planos complexos
    padded.convertTo(padded, CV_32F);
    cv::Mat planes[] = {padded, cv::Mat::zeros(padded.size(), CV_32F)};
    cv::merge(planes, 2, complexFFT);

    // Calcular DFT
    cv::dft(complexFFT, complexFFT);
}

void FFTFilterDialog::computeMagnitudeSpectrum() {
    std::vector<cv::Mat> planes;
    cv::split(complexFFT, planes);

    // Calcular magnitude
    cv::Mat magnitude;
    cv::magnitude(planes[0], planes[1], magnitude);

    // Escala logarítmica para melhor visualização
    magnitude += cv::Scalar::all(1);
    cv::log(magnitude, magnitude);

    // Centralizar espectro (trocar quadrantes)
    magnitude = shiftFFT(magnitude);

    // Normalizar para 0-255
    cv::normalize(magnitude, magnitudeSpectrum, 0, 255, cv::NORM_MINMAX);
    magnitudeSpectrum.convertTo(magnitudeSpectrum, CV_8U);
}

cv::Mat FFTFilterDialog::shiftFFT(const cv::Mat &img) {
    cv::Mat result = img.clone();
    int cx = result.cols / 2;
    int cy = result.rows / 2;

    cv::Mat q0(result, cv::Rect(0, 0, cx, cy));   // Top-Left
    cv::Mat q1(result, cv::Rect(cx, 0, cx, cy));  // Top-Right
    cv::Mat q2(result, cv::Rect(0, cy, cx, cy));  // Bottom-Left
    cv::Mat q3(result, cv::Rect(cx, cy, cx, cy)); // Bottom-Right

    cv::Mat tmp;
    q0.copyTo(tmp);
    q3.copyTo(q0);
    tmp.copyTo(q3);

    q1.copyTo(tmp);
    q2.copyTo(q1);
    tmp.copyTo(q2);

    return result;
}

void FFTFilterDialog::onMaskChanged() {
    updatePreview();
}

void FFTFilterDialog::onInvertMaskChanged(int state) {
    invertMask = (state == Qt::Checked);
    spectrumLabel->setInvertMask(invertMask);
    updatePreview();
}

void FFTFilterDialog::onClearMasks() {
    spectrumLabel->clearMasks();
    filteredImage = originalImage.clone();
    previewViewer->setImage(filteredImage);
}

void FFTFilterDialog::updatePreview() {
    QVector<QRect> rects = spectrumLabel->getMaskRects();

    if (rects.isEmpty()) {
        filteredImage = originalImage.clone();
    } else {
        // Criar máscara
        cv::Mat mask = createMaskFromRects(rects, complexFFT.size());

        // Aplicar máscara na FFT
        cv::Mat filteredFFT = applyFFTMask(complexFFT, mask);

        // IFFT
        cv::Mat ifft;
        cv::idft(filteredFFT, ifft, cv::DFT_SCALE | cv::DFT_REAL_OUTPUT);

        // Extrair parte real e cortar ao tamanho original
        ifft = ifft(cv::Rect(0, 0, originalImage.cols, originalImage.rows));

        // Normalizar e converter
        cv::normalize(ifft, filteredImage, 0, 255, cv::NORM_MINMAX);
        filteredImage.convertTo(filteredImage, CV_8U);

        // Se original era colorido, converter de volta
        if (originalImage.channels() == 3) {
            cv::cvtColor(filteredImage, filteredImage, cv::COLOR_GRAY2BGR);
        }
    }

    previewViewer->setImage(filteredImage);
}

cv::Mat FFTFilterDialog::createMaskFromRects(const QVector<QRect> &rects, const cv::Size &size) {
    // Criar máscara com todos 1s
    cv::Mat mask = cv::Mat::ones(size, CV_32F);

    // Descentrar os retângulos (inverter shift)
    int cx = size.width / 2;
    int cy = size.height / 2;

    for (const QRect &rect : rects) {
        // Converter coordenadas de centralizado para normal
        cv::Rect shiftedRect = cv::Rect(rect.x(), rect.y(), rect.width(), rect.height());

        // Aplicar shift reverso (4 quadrantes)
        std::vector<cv::Rect> quadrants;

        // Quadrante onde o retângulo está
        if (shiftedRect.x < cx && shiftedRect.y < cy) {
            // Top-Left -> Bottom-Right
            quadrants.push_back(cv::Rect(shiftedRect.x + cx, shiftedRect.y + cy, shiftedRect.width, shiftedRect.height));
        }
        if (shiftedRect.x >= cx && shiftedRect.y < cy) {
            // Top-Right -> Bottom-Left
            quadrants.push_back(cv::Rect(shiftedRect.x - cx, shiftedRect.y + cy, shiftedRect.width, shiftedRect.height));
        }
        if (shiftedRect.x < cx && shiftedRect.y >= cy) {
            // Bottom-Left -> Top-Right
            quadrants.push_back(cv::Rect(shiftedRect.x + cx, shiftedRect.y - cy, shiftedRect.width, shiftedRect.height));
        }
        if (shiftedRect.x >= cx && shiftedRect.y >= cy) {
            // Bottom-Right -> Top-Left
            quadrants.push_back(cv::Rect(shiftedRect.x - cx, shiftedRect.y - cy, shiftedRect.width, shiftedRect.height));
        }

        // Zerar (remover) ou manter as regiões
        for (const cv::Rect &qr : quadrants) {
            cv::Rect clipped = qr & cv::Rect(0, 0, size.width, size.height);
            if (clipped.area() > 0) {
                if (invertMask) {
                    // Inverter: começar com zeros e manter apenas seleção
                    // Implementação simplificada - na prática precisa de lógica mais complexa
                    mask(clipped) = 1.0;
                } else {
                    mask(clipped) = 0.0;
                }
            }
        }
    }

    // Se invertMask, precisamos inverter toda a máscara
    if (invertMask && !rects.isEmpty()) {
        cv::Mat tempMask = cv::Mat::zeros(size, CV_32F);
        for (const QRect &rect : rects) {
            cv::Rect shiftedRect(rect.x(), rect.y(), rect.width(), rect.height());
            // Aplicar lógica similar mas setando 1 na seleção
            std::vector<cv::Rect> quadrants;

            if (shiftedRect.x < cx && shiftedRect.y < cy) {
                quadrants.push_back(cv::Rect(shiftedRect.x + cx, shiftedRect.y + cy, shiftedRect.width, shiftedRect.height));
            }
            if (shiftedRect.x >= cx && shiftedRect.y < cy) {
                quadrants.push_back(cv::Rect(shiftedRect.x - cx, shiftedRect.y + cy, shiftedRect.width, shiftedRect.height));
            }
            if (shiftedRect.x < cx && shiftedRect.y >= cy) {
                quadrants.push_back(cv::Rect(shiftedRect.x + cx, shiftedRect.y - cy, shiftedRect.width, shiftedRect.height));
            }
            if (shiftedRect.x >= cx && shiftedRect.y >= cy) {
                quadrants.push_back(cv::Rect(shiftedRect.x - cx, shiftedRect.y - cy, shiftedRect.width, shiftedRect.height));
            }

            for (const cv::Rect &qr : quadrants) {
                cv::Rect clipped = qr & cv::Rect(0, 0, size.width, size.height);
                if (clipped.area() > 0) {
                    tempMask(clipped) = 1.0;
                }
            }
        }
        mask = tempMask;
    }

    return mask;
}

cv::Mat FFTFilterDialog::applyFFTMask(const cv::Mat &complexImg, const cv::Mat &mask) {
    std::vector<cv::Mat> planes;
    cv::split(complexImg, planes);

    // Aplicar máscara nos canais real e imaginário
    planes[0] = planes[0].mul(mask);
    planes[1] = planes[1].mul(mask);

    cv::Mat result;
    cv::merge(planes, result);

    return result;
}

void FFTFilterDialog::onAccept() {
    accepted = true;
    accept();
}

void FFTFilterDialog::onReject() {
    accepted = false;
    reject();
}
