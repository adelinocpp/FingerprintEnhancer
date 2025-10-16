#include "FragmentRegionsOverlay.h"
#include "../core/ProjectModel.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <cmath>

FragmentRegionsOverlay::FragmentRegionsOverlay(QWidget *parent)
    : QWidget(parent)
    , currentImage(nullptr)
    , scaleFactor(1.0)
    , scrollOffset(0, 0)
    , imageOffset(0, 0)
    , showRegions(false)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // Garantir que o widget está visível e no topo
    setVisible(true);
    raise();
}

void FragmentRegionsOverlay::setImage(FingerprintEnhancer::FingerprintImage* img) {
    currentImage = img;
    update();
}

void FragmentRegionsOverlay::setScaleFactor(double factor) {
    scaleFactor = factor;
    update();
}

void FragmentRegionsOverlay::setScrollOffset(const QPoint& offset) {
    scrollOffset = offset;
    update();
}

void FragmentRegionsOverlay::setImageOffset(const QPoint& offset) {
    imageOffset = offset;
    update();
}

void FragmentRegionsOverlay::setVisible(bool visible) {
    QWidget::setVisible(visible);
    update();
}

void FragmentRegionsOverlay::setShowRegions(bool show) {
    showRegions = show;
    
    if (show) {
        // Forçar visibilidade e trazer para frente
        setVisible(true);
        raise();
        activateWindow();
    }
    
    update();
    repaint();  // Forçar repaint imediato
}

void FragmentRegionsOverlay::paintEvent(QPaintEvent *event) {
    if (!showRegions || !currentImage || currentImage->fragments.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Desenhar cada fragmento
    for (int i = 0; i < currentImage->fragments.size(); ++i) {
        const auto& fragment = currentImage->fragments[i];
        drawFragmentRegion(painter, &fragment, i + 1);  // Número do fragmento (1-indexed)
    }
}

void FragmentRegionsOverlay::drawFragmentRegion(QPainter& painter, 
                                                 const FingerprintEnhancer::Fragment* fragment, 
                                                 int index) {
    if (!fragment || !currentImage) return;

    // Calcular diferença de rotação entre quando fragmento foi criado e agora
    double rotationDiff = currentImage->currentRotationAngle - fragment->sourceRotationAngle;
    
    // Se não há rotação, desenhar normalmente
    if (fabs(rotationDiff) < 0.1) {
        // Obter sourceRect (coordenadas na imagem original)
        QRect sourceRect = fragment->sourceRect;
        
        // Converter para coordenadas do widget (escaladas e com offset)
        QRectF scaledRect(
            sourceRect.x() * scaleFactor + imageOffset.x() - scrollOffset.x(),
            sourceRect.y() * scaleFactor + imageOffset.y() - scrollOffset.y(),
            sourceRect.width() * scaleFactor,
            sourceRect.height() * scaleFactor
        );
        
        drawRectAndLabel(painter, scaledRect, index);
        return;
    }
    
    // Imagem foi rotacionada - calcular novas coordenadas
    QRect sourceRect = fragment->sourceRect;
    cv::Size originalSize(currentImage->originalImage.cols, currentImage->originalImage.rows);
    cv::Size currentSize(currentImage->workingImage.cols, currentImage->workingImage.rows);
    
    // Calcular centro da imagem original
    double centerX = originalSize.width / 2.0;
    double centerY = originalSize.height / 2.0;
    
    // Converter ângulo para radianos
    double angleRad = -rotationDiff * M_PI / 180.0;  // Negativo porque Qt usa sentido anti-horário
    double cosA = cos(angleRad);
    double sinA = sin(angleRad);
    
    // Rotacionar os 4 cantos do retângulo original
    QPointF corners[4] = {
        QPointF(sourceRect.x(), sourceRect.y()),
        QPointF(sourceRect.x() + sourceRect.width(), sourceRect.y()),
        QPointF(sourceRect.x() + sourceRect.width(), sourceRect.y() + sourceRect.height()),
        QPointF(sourceRect.x(), sourceRect.y() + sourceRect.height())
    };
    
    QPolygonF rotatedPoly;
    for (int i = 0; i < 4; i++) {
        // Transladar para origem
        double x = corners[i].x() - centerX;
        double y = corners[i].y() - centerY;
        
        // Rotacionar
        double xRot = x * cosA - y * sinA;
        double yRot = x * sinA + y * cosA;
        
        // Ajustar para o novo centro (pode ter mudado se rotação de 90°)
        double newCenterX = currentSize.width / 2.0;
        double newCenterY = currentSize.height / 2.0;
        
        // Transladar de volta
        double xFinal = xRot + newCenterX;
        double yFinal = yRot + newCenterY;
        
        // Escalar e aplicar offsets
        rotatedPoly << QPointF(
            xFinal * scaleFactor + imageOffset.x() - scrollOffset.x(),
            yFinal * scaleFactor + imageOffset.y() - scrollOffset.y()
        );
    }
    
    // Desenhar polígono ao invés de retângulo
    QColor borderColor(0, 180, 255, 255);
    painter.setPen(QPen(borderColor, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(rotatedPoly);
    
    // Desenhar rótulo no primeiro canto
    QPointF labelPos = rotatedPoly[0];
    drawLabel(painter, labelPos, index);
}

void FragmentRegionsOverlay::drawRectAndLabel(QPainter& painter, const QRectF& rect, int index) {
    // Desenhar apenas BORDA do retângulo (sem preenchimento)
    QColor borderColor(0, 180, 255, 255);
    painter.setPen(QPen(borderColor, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect);
    
    // Desenhar rótulo no canto superior esquerdo
    drawLabel(painter, QPointF(rect.left(), rect.top()), index);
}

void FragmentRegionsOverlay::drawLabel(QPainter& painter, const QPointF& pos, int index) {
    QString label = QString("F%1").arg(index);
    
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(label);
    
    // Posição do rótulo
    int labelX = pos.x() + 5;
    int labelY = pos.y() + fm.height() + 3;
    
    // Desenhar fundo do rótulo
    QRect labelBg(labelX - 3, labelY - fm.height() - 1, textRect.width() + 6, fm.height() + 4);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 150, 255, 220));
    painter.drawRoundedRect(labelBg, 3, 3);
    
    // Desenhar texto
    painter.setPen(Qt::white);
    painter.drawText(labelX, labelY - 2, label);
}
