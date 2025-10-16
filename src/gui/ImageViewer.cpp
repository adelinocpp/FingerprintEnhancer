#include "ImageViewer.h"
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollBar>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QRegion>
#include <QtGui/QPolygon>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtCore/QEvent>
#include <QtCore/QDebug>
#include <QtCore/QSettings>

// ==================== CropOverlayLabel ====================

CropOverlayLabel::CropOverlayLabel(ImageViewer *viewer, QWidget *parent)
    : QLabel(parent), imageViewer(viewer), overlayEnabled(false) {
}

void CropOverlayLabel::setOverlayRect(const QRect &rect) {
    overlayRect = rect;
    update();
}

void CropOverlayLabel::clearOverlay() {
    overlayRect = QRect();
    update();
}

void CropOverlayLabel::setOverlayEnabled(bool enabled) {
    overlayEnabled = enabled;
    update();
}

void CropOverlayLabel::paintEvent(QPaintEvent *event) {
    // Primeiro desenha a imagem normalmente
    QLabel::paintEvent(event);

    // Depois desenha o overlay se estiver habilitado
    if (overlayEnabled && imageViewer) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        double scaleFactor = imageViewer->getScaleFactor();
        QRect cropSelection = imageViewer->getCropSelectionInternal();

        if (!cropSelection.isNull()) {
            // Converter coordenadas da imagem para coordenadas do widget (label)
            QRect widgetRect(
                cropSelection.x() * scaleFactor,
                cropSelection.y() * scaleFactor,
                cropSelection.width() * scaleFactor,
                cropSelection.height() * scaleFactor
            );

            // Desenhar overlay escuro fora da seleção
            QRegion outsideRegion(rect());
            outsideRegion = outsideRegion.subtracted(QRegion(widgetRect));
            painter.setClipRegion(outsideRegion);
            painter.fillRect(rect(), QColor(0, 0, 0, 100));
            painter.setClipping(false);

            // Desenhar bordas duplas para melhor visibilidade
            // Borda externa branca
            QPen whitePen(Qt::white, 3, Qt::DashLine);
            whitePen.setDashPattern({8, 4});
            painter.setPen(whitePen);
            painter.drawRect(widgetRect.adjusted(-1, -1, 1, 1));

            // Borda interna preta
            QPen blackPen(Qt::black, 2, Qt::DashLine);
            blackPen.setDashPattern({8, 4});
            painter.setPen(blackPen);
            painter.drawRect(widgetRect);

            // Desenhar alças de redimensionamento nos cantos
            int handleSize = 8;
            QBrush handleBrush(Qt::white);
            painter.setBrush(handleBrush);
            painter.setPen(QPen(Qt::black, 1));

            // Cantos (quadrados)
            painter.drawRect(widgetRect.topLeft().x() - handleSize/2,
                            widgetRect.topLeft().y() - handleSize/2, handleSize, handleSize);
            painter.drawRect(widgetRect.topRight().x() - handleSize/2,
                            widgetRect.topRight().y() - handleSize/2, handleSize, handleSize);
            painter.drawRect(widgetRect.bottomLeft().x() - handleSize/2,
                            widgetRect.bottomLeft().y() - handleSize/2, handleSize, handleSize);
            painter.drawRect(widgetRect.bottomRight().x() - handleSize/2,
                            widgetRect.bottomRight().y() - handleSize/2, handleSize, handleSize);
            
            // Bordas (losangos/diamantes sobre a linha)
            int diamondSize = handleSize + 4;
            
            // Calcular centros das bordas
            int centerX = (widgetRect.left() + widgetRect.right()) / 2;
            int centerY = (widgetRect.top() + widgetRect.bottom()) / 2;
            
            // Desenhar losango no topo (sobre a borda superior)
            QPolygon topDiamond;
            int half = diamondSize / 2;
            topDiamond << QPoint(centerX, widgetRect.top() - half)
                       << QPoint(centerX + half, widgetRect.top())
                       << QPoint(centerX, widgetRect.top() + half)
                       << QPoint(centerX - half, widgetRect.top());
            painter.drawPolygon(topDiamond);
            
            // Desenhar losango na base (sobre a borda inferior)
            QPolygon bottomDiamond;
            bottomDiamond << QPoint(centerX, widgetRect.bottom() - half)
                          << QPoint(centerX + half, widgetRect.bottom())
                          << QPoint(centerX, widgetRect.bottom() + half)
                          << QPoint(centerX - half, widgetRect.bottom());
            painter.drawPolygon(bottomDiamond);
            
            // Desenhar losango à esquerda (sobre a borda esquerda)
            QPolygon leftDiamond;
            leftDiamond << QPoint(widgetRect.left(), centerY - half)
                        << QPoint(widgetRect.left() + half, centerY)
                        << QPoint(widgetRect.left(), centerY + half)
                        << QPoint(widgetRect.left() - half, centerY);
            painter.drawPolygon(leftDiamond);
            
            // Desenhar losango à direita (sobre a borda direita)
            QPolygon rightDiamond;
            rightDiamond << QPoint(widgetRect.right() - half, centerY)
                         << QPoint(widgetRect.right(), centerY - half)
                         << QPoint(widgetRect.right() + half, centerY)
                         << QPoint(widgetRect.right(), centerY + half);
            painter.drawPolygon(rightDiamond);

            // Desenhar dimensões com fundo
            QString dimensions = QString("%1 x %2 px")
                .arg(cropSelection.width())
                .arg(cropSelection.height());

            QFont font("Arial", 12, QFont::Bold);
            painter.setFont(font);
            QFontMetrics fm(font);
            QRect textRect = fm.boundingRect(dimensions);
            textRect.adjust(-5, -3, 5, 3);
            textRect.moveTopLeft(widgetRect.topLeft() + QPoint(10, 10));

            // Fundo semi-transparente
            painter.fillRect(textRect, QColor(0, 0, 0, 180));

            // Texto
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter, dimensions);
        }
    }
}

// ==================== ImageViewer ====================

const double ImageViewer::MIN_ZOOM_FACTOR = 0.1;
const double ImageViewer::MAX_ZOOM_FACTOR = 10.0;
const double ImageViewer::ZOOM_STEP = 1.2;

ImageViewer::ImageViewer(QWidget *parent)
    : QScrollArea(parent), scaleFactor(1.0), panning(false), syncViewer(nullptr), syncEnabled(true),
      cropModeEnabled(false), isSelecting(false), isMovingSelection(false), isResizingEdge(false),
      activeEdgeHandle(EDGE_NONE) {
    imageLabel = new CropOverlayLabel(this);
    setFocusPolicy(Qt::StrongFocus);  // Permitir receber eventos de teclado
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);
    setWidget(imageLabel);
    setAlignment(Qt::AlignCenter);

    // Habilitar mouse tracking para recorte
    setMouseTracking(true);

    // Conectar scrollbars para emitir sinal quando scroll mudar
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &ImageViewer::onScrollBarValueChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &ImageViewer::onScrollBarValueChanged);
}

ImageViewer::~ImageViewer() = default;

void ImageViewer::setImage(const cv::Mat &image) {
    if (image.empty()) {
        clearImage();
        return;
    }
    
    currentImage = image.clone();
    currentPixmap = matToQPixmap(image);
    updateImageDisplay();
}

void ImageViewer::setPixmap(const QPixmap &pixmap) {
    currentPixmap = pixmap;
    updateImageDisplay();
}

void ImageViewer::clearImage() {
    currentImage = cv::Mat();
    currentPixmap = QPixmap();
    imageLabel->clear();
    imageLabel->setText("No image loaded");
}

void ImageViewer::zoomIn() {
    scaleImage(ZOOM_STEP);
}

void ImageViewer::zoomOut() {
    scaleImage(1.0 / ZOOM_STEP);
}

void ImageViewer::zoomToFit() {
    if (currentPixmap.isNull()) return;
    
    QSize viewportSize = viewport()->size();
    QSize pixmapSize = currentPixmap.size();
    
    double scaleX = static_cast<double>(viewportSize.width()) / pixmapSize.width();
    double scaleY = static_cast<double>(viewportSize.height()) / pixmapSize.height();
    
    double scale = std::min(scaleX, scaleY);
    setZoomFactor(scale);
}

void ImageViewer::zoomToActual() {
    setZoomFactor(1.0);
}

void ImageViewer::setZoomFactor(double factor) {
    scaleFactor = std::max(MIN_ZOOM_FACTOR, std::min(MAX_ZOOM_FACTOR, factor));
    updateImageDisplay();
    emit zoomChanged(scaleFactor);
}

void ImageViewer::setSyncViewer(ImageViewer *viewer) {
    syncViewer = viewer;
}

void ImageViewer::syncViewport(const QPoint &position) {
    if (syncViewer && syncEnabled) {
        syncViewer->horizontalScrollBar()->setValue(position.x());
        syncViewer->verticalScrollBar()->setValue(position.y());
    }
}

bool ImageViewer::hasImage() const {
    return !currentImage.empty() || !currentPixmap.isNull();
}

cv::Mat ImageViewer::getCurrentImage() const {
    return currentImage;
}

QSize ImageViewer::getImageSize() const {
    if (!currentPixmap.isNull()) {
        return currentPixmap.size();
    }
    return QSize();
}

QPoint ImageViewer::imageToWidget(const QPoint &imagePoint) const {
    return QPoint(static_cast<int>(imagePoint.x() * scaleFactor),
                  static_cast<int>(imagePoint.y() * scaleFactor));
}

QPoint ImageViewer::widgetToImage(const QPoint &widgetPoint) const {
    // Converter de coordenadas do viewport para coordenadas do label
    // Precisamos considerar o scroll offset
    QPoint labelPos = imageLabel->mapFromGlobal(viewport()->mapToGlobal(widgetPoint));

    // Agora converter de coordenadas do label (escaladas) para coordenadas da imagem original
    return QPoint(static_cast<int>(labelPos.x() / scaleFactor),
                  static_cast<int>(labelPos.y() / scaleFactor));
}

void ImageViewer::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
    event->accept();
}

void ImageViewer::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPoint imagePos = widgetToImage(event->pos());
        QPoint labelPos = imageLabel->mapFromGlobal(viewport()->mapToGlobal(event->pos()));

        if (cropModeEnabled) {
            // 1. Verificar se clicou em losango (handle de redimensionamento)
            EdgeHandle handle = getEdgeHandleAtPoint(labelPos);
            
            if (handle != EDGE_NONE && !cropSelection.isNull()) {
                // Redimensionar borda específica
                isResizingEdge = true;
                activeEdgeHandle = handle;
                cropStart = imagePos;
                event->accept();
                return;
            }
            
            // 2. Verificar se clicou DENTRO da seleção (mas não no losango)
            if (!cropSelection.isNull() && cropSelection.contains(imagePos)) {
                // Mover seleção inteira
                isMovingSelection = true;
                cropStart = imagePos;
                setCursor(Qt::ClosedHandCursor);
                event->accept();
                return;
            }
            
            // 3. Clicou fora: iniciar nova seleção
            isSelecting = true;
            cropStart = imagePos;
            cropEnd = imagePos;
            cropSelection = QRect();
        } else {
            // Modo normal: panning
            panning = true;
            lastPanPoint = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }

        emit imageClicked(imagePos);
    }
    QScrollArea::mousePressEvent(event);
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
    QPoint imagePos = widgetToImage(event->pos());

    // Mover seleção inteira (clicou e está arrastando de dentro da seleção)
    if (cropModeEnabled && isMovingSelection && !cropSelection.isNull()) {
        QPoint delta = imagePos - cropStart;
        QRect newSelection = cropSelection.translated(delta);
        
        // Limitar à área da imagem (clamping)
        if (!currentImage.empty()) {
            QRect imageRect(0, 0, currentImage.cols, currentImage.rows);
            
            // Garantir que a seleção não saia dos limites da imagem
            if (newSelection.left() < 0) {
                newSelection.moveLeft(0);
            }
            if (newSelection.top() < 0) {
                newSelection.moveTop(0);
            }
            if (newSelection.right() >= currentImage.cols) {
                newSelection.moveRight(currentImage.cols - 1);
            }
            if (newSelection.bottom() >= currentImage.rows) {
                newSelection.moveBottom(currentImage.rows - 1);
            }
        }
        
        cropSelection = newSelection;
        cropStart = imagePos;  // Atualizar posição de referência
        imageLabel->setOverlayRect(cropSelection);
        emit cropSelectionChanged(cropSelection);
    }
    // Redimensionar borda específica (arrastando losango)
    else if (cropModeEnabled && isResizingEdge && activeEdgeHandle != EDGE_NONE) {
        QRect newSelection = cropSelection;
        
        switch (activeEdgeHandle) {
            case EDGE_TOP:
                newSelection.setTop(imagePos.y());
                break;
            case EDGE_BOTTOM:
                newSelection.setBottom(imagePos.y());
                break;
            case EDGE_LEFT:
                newSelection.setLeft(imagePos.x());
                break;
            case EDGE_RIGHT:
                newSelection.setRight(imagePos.x());
                break;
            default:
                break;
        }
        
        // Normalizar e validar
        newSelection = newSelection.normalized();
        if (newSelection.width() >= 10 && newSelection.height() >= 10) {
            // Limitar à área da imagem
            if (!currentImage.empty()) {
                QRect imageRect(0, 0, currentImage.cols, currentImage.rows);
                cropSelection = newSelection.intersected(imageRect);
            } else {
                cropSelection = newSelection;
            }
            imageLabel->setOverlayRect(cropSelection);
            emit cropSelectionChanged(cropSelection);
        }
    }
    // Desenhar nova seleção
    else if (cropModeEnabled && isSelecting) {
        // Atualizar seleção de recorte
        cropEnd = imagePos;
        QRect newSelection = QRect(cropStart, cropEnd).normalized();

        // Limitar à área da imagem
        if (!currentImage.empty()) {
            QRect imageRect(0, 0, currentImage.cols, currentImage.rows);
            cropSelection = newSelection.intersected(imageRect);
        } else {
            cropSelection = newSelection;
        }

        // Atualizar overlay no label
        imageLabel->setOverlayRect(cropSelection);
        emit cropSelectionChanged(cropSelection);
    }
    // Panning normal
    else if (panning) {
        QPoint delta = event->pos() - lastPanPoint;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        lastPanPoint = event->pos();

        QPoint viewportPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
        emit viewportChanged(viewportPos);
        syncViewport(viewportPos);
    }

    emit mouseMoved(imagePos);
    QScrollArea::mouseMoveEvent(event);
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (cropModeEnabled && isSelecting) {
            isSelecting = false;
            if (!cropSelection.isNull()) {
                emit cropSelectionChanged(cropSelection);
            }
        }
        
        // Finalizar movimento de seleção
        if (isMovingSelection) {
            isMovingSelection = false;
            if (!cropSelection.isNull()) {
                emit cropSelectionChanged(cropSelection);
            }
        }
        
        // Finalizar redimensionamento de borda
        if (isResizingEdge) {
            isResizingEdge = false;
            activeEdgeHandle = EDGE_NONE;
            if (!cropSelection.isNull()) {
                emit cropSelectionChanged(cropSelection);
            }
        }
        
        panning = false;
        setCursor(Qt::ArrowCursor);
    }
    QScrollArea::mouseReleaseEvent(event);
}

void ImageViewer::resizeEvent(QResizeEvent *event) {
    QScrollArea::resizeEvent(event);
    // Emitir novo offset quando viewport redimensionar
    emit imageOffsetChanged(getImageOffset());
}

void ImageViewer::onSyncViewportChanged(QPoint position) {
    if (syncEnabled) {
        horizontalScrollBar()->setValue(position.x());
        verticalScrollBar()->setValue(position.y());
    }
}

void ImageViewer::updateImageDisplay() {
    if (currentPixmap.isNull()) return;

    QPixmap scaledPixmap = currentPixmap.scaled(
        currentPixmap.size() * scaleFactor,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    imageLabel->setPixmap(scaledPixmap);
    imageLabel->resize(scaledPixmap.size());

    // Emitir novo offset quando display atualizar
    emit imageOffsetChanged(getImageOffset());
}

void ImageViewer::scaleImage(double factor) {
    setZoomFactor(scaleFactor * factor);
}

QPixmap ImageViewer::matToQPixmap(const cv::Mat &mat) {
    if (mat.empty()) return QPixmap();
    
    cv::Mat display;
    if (mat.channels() == 1) {
        cv::cvtColor(mat, display, cv::COLOR_GRAY2RGB);
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, display, cv::COLOR_BGR2RGB);
    } else {
        display = mat;
    }
    
    QImage qimg(display.data, display.cols, display.rows, display.step, QImage::Format_RGB888);
    return QPixmap::fromImage(qimg);
}

void ImageViewer::adjustScrollBars(double factor) {
    // Implementação básica para ajuste das barras de rolagem
}

void ImageViewer::centerImage() {
    // Implementação básica para centralizar a imagem
}

// ==================== FUNÇÕES DE RECORTE ====================

void ImageViewer::setCropMode(bool enabled) {
    cropModeEnabled = enabled;
    if (!enabled) {
        clearCropSelection();
        isSelecting = false;
        isMovingSelection = false;
        isResizingEdge = false;
        activeEdgeHandle = EDGE_NONE;
        setCursor(Qt::ArrowCursor);
    }
    imageLabel->setOverlayEnabled(enabled);
    
    // Garantir que o viewport recebe os eventos
    if (enabled) {
        viewport()->setFocus();
        setFocus();
    }
}

void ImageViewer::setCropSelection(const QRect& rect) {
    cropSelection = rect;
    if (cropModeEnabled && !rect.isNull()) {
        // Converter para coordenadas do widget para exibir
        QRect widgetRect(
            imageToWidget(rect.topLeft()),
            imageToWidget(rect.bottomRight())
        );
        imageLabel->setOverlayRect(widgetRect);
        emit cropSelectionChanged(rect);
    }
}

void ImageViewer::clearCropSelection() {
    cropSelection = QRect();
    isSelecting = false;
    imageLabel->clearOverlay();
}

void ImageViewer::saveCropSelectionState(const QString& imageId) {
    if (imageId.isEmpty()) {
        return;
    }
    
    QSettings settings("FingerprintEnhancer", "CropSelections");
    settings.beginGroup(imageId);
    
    if (!cropSelection.isNull()) {
        // Salvar seleção como array: [x, y, width, height]
        settings.setValue("version", 1);  // Versão do formato
        settings.setValue("hasCrop", true);
        settings.setValue("x", cropSelection.x());
        settings.setValue("y", cropSelection.y());
        settings.setValue("width", cropSelection.width());
        settings.setValue("height", cropSelection.height());
    } else {
        // Limpar seleção salva
        settings.setValue("version", 1);
        settings.setValue("hasCrop", false);
    }
    
    settings.endGroup();
    settings.sync();
}

void ImageViewer::restoreCropSelectionState(const QString& imageId) {
    if (imageId.isEmpty()) {
        return;
    }
    
    QSettings settings("FingerprintEnhancer", "CropSelections");
    settings.beginGroup(imageId);
    
    // Compatibilidade retroativa: se não existir "version", arquivo é de versão antiga
    int version = settings.value("version", 0).toInt();
    
    if (version == 0) {
        // Versão antiga (sem crop selection salva) - não fazer nada
        settings.endGroup();
        return;
    }
    
    // Versão 1 ou superior
    bool hasCrop = settings.value("hasCrop", false).toBool();
    
    if (hasCrop) {
        int x = settings.value("x", 0).toInt();
        int y = settings.value("y", 0).toInt();
        int width = settings.value("width", 0).toInt();
        int height = settings.value("height", 0).toInt();
        
        // Validar coordenadas
        if (width > 0 && height > 0 && !currentImage.empty()) {
            // Garantir que está dentro dos limites da imagem
            if (x >= 0 && y >= 0 && 
                x + width <= currentImage.cols && 
                y + height <= currentImage.rows) {
                cropSelection = QRect(x, y, width, height);
                imageLabel->setOverlayRect(cropSelection);
            }
        }
    }
    
    settings.endGroup();
}

void ImageViewer::onScrollBarValueChanged() {
    // Emitir sinal com o offset de scroll atual
    QPoint scrollOffset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    emit scrollChanged(scrollOffset);
}

QPoint ImageViewer::getImageOffset() const {
    if (currentPixmap.isNull()) {
        return QPoint(0, 0);
    }

    // Tamanho da imagem escalada
    QSize scaledSize = currentPixmap.size() * scaleFactor;

    // Tamanho do viewport
    QSize viewportSize = viewport()->size();

    // Calcular offset de centralização
    int offsetX = 0;
    int offsetY = 0;

    if (scaledSize.width() < viewportSize.width()) {
        offsetX = (viewportSize.width() - scaledSize.width()) / 2;
    }

    if (scaledSize.height() < viewportSize.height()) {
        offsetY = (viewportSize.height() - scaledSize.height()) / 2;
    }

    return QPoint(offsetX, offsetY);
}

void ImageViewer::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (cropModeEnabled && !cropSelection.isNull()) {
            // ESC cancela a seleção inteira
            clearCropSelection();
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }
    }
    QScrollArea::keyPressEvent(event);
}

ImageViewer::EdgeHandle ImageViewer::getEdgeHandleAtPoint(const QPoint &widgetPos) const {
    if (cropSelection.isNull()) {
        return EDGE_NONE;
    }
    
    // Converter seleção para coordenadas do widget
    QRect widgetRect(
        cropSelection.x() * scaleFactor,
        cropSelection.y() * scaleFactor,
        cropSelection.width() * scaleFactor,
        cropSelection.height() * scaleFactor
    );
    
    int handleSize = 12;  // Tamanho do losango
    int centerX = (widgetRect.left() + widgetRect.right()) / 2;
    int centerY = (widgetRect.top() + widgetRect.bottom()) / 2;
    
    // Verificar losango no topo
    if (isPointInDiamondHandle(widgetPos, QPoint(centerX, widgetRect.top()), handleSize)) {
        return EDGE_TOP;
    }
    
    // Verificar losango na base
    if (isPointInDiamondHandle(widgetPos, QPoint(centerX, widgetRect.bottom()), handleSize)) {
        return EDGE_BOTTOM;
    }
    
    // Verificar losango à esquerda
    if (isPointInDiamondHandle(widgetPos, QPoint(widgetRect.left(), centerY), handleSize)) {
        return EDGE_LEFT;
    }
    
    // Verificar losango à direita
    if (isPointInDiamondHandle(widgetPos, QPoint(widgetRect.right(), centerY), handleSize)) {
        return EDGE_RIGHT;
    }
    
    return EDGE_NONE;
}

bool ImageViewer::isPointInDiamondHandle(const QPoint &widgetPos, const QPoint &handleCenter, int size) const {
    // Verificar se o ponto está dentro do losango usando distância Manhattan
    int dx = qAbs(widgetPos.x() - handleCenter.x());
    int dy = qAbs(widgetPos.y() - handleCenter.y());
    int half = size / 2;
    
    // Ponto está dentro do losango se a soma das distâncias é menor que o raio
    return (dx + dy) <= half + 2;  // +2 para tolerância
}

