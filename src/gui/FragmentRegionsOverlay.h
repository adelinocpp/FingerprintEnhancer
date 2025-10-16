#ifndef FRAGMENTREGIONSOVERLAY_H
#define FRAGMENTREGIONSOVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QList>

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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    FingerprintEnhancer::FingerprintImage* currentImage;
    double scaleFactor;
    QPoint scrollOffset;
    QPoint imageOffset;
    bool showRegions;
    
    void drawFragmentRegion(QPainter& painter, const FingerprintEnhancer::Fragment* fragment, int index);
    void drawRectAndLabel(QPainter& painter, const QRectF& rect, int index);
    void drawLabel(QPainter& painter, const QPointF& pos, int index);
};

#endif // FRAGMENTREGIONSOVERLAY_H
