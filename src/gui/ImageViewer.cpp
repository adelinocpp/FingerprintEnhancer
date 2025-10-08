#include "ImageViewer.h"
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollBar>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QRegion>
#include <QtCore/QDebug>

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
    if (overlayEnabled && imageViewer && !overlayRect.isNull()) {
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

            // Cantos
            painter.drawRect(widgetRect.topLeft().x() - handleSize/2,
                            widgetRect.topLeft().y() - handleSize/2, handleSize, handleSize);
            painter.drawRect(widgetRect.topRight().x() - handleSize/2,
                            widgetRect.topRight().y() - handleSize/2, handleSize, handleSize);
            painter.drawRect(widgetRect.bottomLeft().x() - handleSize/2,
                            widgetRect.bottomLeft().y() - handleSize/2, handleSize, handleSize);
            painter.drawRect(widgetRect.bottomRight().x() - handleSize/2,
                            widgetRect.bottomRight().y() - handleSize/2, handleSize, handleSize);

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
      cropModeEnabled(false), isSelecting(false) {
    imageLabel = new CropOverlayLabel(this);
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);
    setWidget(imageLabel);
    setAlignment(Qt::AlignCenter);

    // Habilitar mouse tracking para recorte
    setMouseTracking(true);
    imageLabel->setMouseTracking(true);
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

        if (cropModeEnabled) {
            // Modo de recorte: iniciar seleção
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

    if (cropModeEnabled && isSelecting) {
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
    } else if (panning) {
        // Panning normal
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
        panning = false;
        setCursor(Qt::ArrowCursor);
    }
    QScrollArea::mouseReleaseEvent(event);
}

void ImageViewer::resizeEvent(QResizeEvent *event) {
    QScrollArea::resizeEvent(event);
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
    imageLabel->setOverlayEnabled(enabled);
    if (!enabled) {
        isSelecting = false;
        cropSelection = QRect();
        imageLabel->clearOverlay();
    }
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
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

