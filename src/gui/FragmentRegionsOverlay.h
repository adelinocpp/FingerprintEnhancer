#ifndef FRAGMENTREGIONSOVERLAY_H
#define FRAGMENTREGIONSOVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QList>
#include <QPolygonF>

namespace FingerprintEnhancer {
    struct Fragment;
    class FingerprintImage;
}

/**
 * @brief Overlay para desenhar regiões de origem dos fragmentos sobre a imagem
 */
class FragmentRegionsOverlay : public QWidget {
    Q_OBJECT

public:
    explicit FragmentRegionsOverlay(QWidget *parent = nullptr);

    void setImage(FingerprintEnhancer::FingerprintImage* img);
    void setScaleFactor(double factor);
    void setScrollOffset(const QPoint& offset);
    void setImageOffset(const QPoint& offset);
    void setVisible(bool visible);
    
    bool isShowingRegions() const { return showRegions; }
    void setShowRegions(bool show);
    
    // Para preview de rotação interativa
    void setPreviewRotationAngle(double angle) { previewRotationAngle = angle; update(); }
    void clearPreviewRotationAngle() { previewRotationAngle = -1.0; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    FingerprintEnhancer::FingerprintImage* currentImage;
    double scaleFactor;
    QPoint scrollOffset;
    QPoint imageOffset;
    bool showRegions;
    
    // Ângulo de preview para rotação interativa (sobrescreve currentRotationAngle se >= 0)
    double previewRotationAngle;
    
    void drawFragmentRegion(QPainter& painter, const FingerprintEnhancer::Fragment* fragment, int index);
    void drawRectAndLabel(QPainter& painter, const QRectF& rect, int index);
    void drawPolygonAndLabel(QPainter& painter, const QPolygonF& poly, int index);
    void drawLabel(QPainter& painter, const QPointF& pos, int index);
    
    // Conversão de coordenadas: original → rotacionada atual
    QRect convertOriginalToCurrentCoords(const QRect& originalRect, double currentAngle,
                                         const QSize& originalSize, const QSize& currentSize);
    
    // Calcular polígono rotacionado para ângulos arbitrários
    QPolygonF calculateRotatedPolygon(const QRect& originalRect, double angleDeg,
                                      const QSize& originalSize, const QSize& rotatedSize);
};

#endif // FRAGMENTREGIONSOVERLAY_H
