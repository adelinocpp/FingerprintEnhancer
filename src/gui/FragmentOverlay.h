#ifndef FRAGMENTOVERLAY_H
#define FRAGMENTOVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QRect>
#include <QPoint>
#include "../core/ProjectModel.h"

/**
 * @brief Overlay para visualizar retângulos de fragmentos na imagem original
 * 
 * Similar ao MinutiaeOverlay, mas mostra os retângulos de onde os fragmentos
 * foram recortados, considerando as rotações acumuladas da imagem.
 * 
 * As marcações acompanham as rotações interativas da imagem.
 */
class FragmentOverlay : public QWidget {
    Q_OBJECT

public:
    explicit FragmentOverlay(QWidget *parent = nullptr);
    ~FragmentOverlay();

    // Configuração
    void setFragments(const QVector<FingerprintEnhancer::Fragment>& frags);
    void setCurrentImageRotation(double angle);  // Ângulo atual da imagem (graus)
    void setZoomFactor(double factor);
    void setScrollOffset(const QPoint& offset);
    void setImageOffset(const QPoint& offset);
    void setImageSize(const QSize& size);

    // Visibilidade
    void setFragmentsVisible(bool visible);
    bool areFragmentsVisible() const { return fragmentsVisible; }

    // Seleção
    void setSelectedFragmentId(const QString& fragmentId);
    QString getSelectedFragmentId() const { return selectedFragmentId; }

    // Highlight ao passar mouse
    void setHighlightedFragmentId(const QString& fragmentId);

signals:
    void fragmentClicked(const QString& fragmentId, QPoint imagePosition);
    void fragmentHovered(const QString& fragmentId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // Conversão de coordenadas
    QPoint imageToWidget(const QPointF& imagePoint) const;
    QPointF widgetToImage(const QPoint& widgetPoint) const;
    
    // Transformação considerando rotação
    QPolygonF getFragmentPolygon(const FingerprintEnhancer::Fragment& fragment) const;
    QPointF rotatePoint(const QPointF& point, const QPointF& center, double angleDeg) const;
    QRectF getImageBounds() const;
    
    // Busca
    QString findFragmentAtPosition(const QPoint& widgetPos) const;
    
    // Desenho
    void drawFragment(QPainter& painter, const FingerprintEnhancer::Fragment& fragment);
    void drawFragmentLabel(QPainter& painter, const FingerprintEnhancer::Fragment& fragment, 
                          const QPolygonF& polygon);

    // Dados
    QVector<FingerprintEnhancer::Fragment> fragments;
    double currentImageRotation;  // Rotação atual da imagem (graus)
    bool fragmentsVisible;
    QString selectedFragmentId;
    QString highlightedFragmentId;
    
    // Geometria
    double zoomFactor;
    QPoint scrollOffset;
    QPoint imageOffset;
    QSize imageSize;
};

#endif // FRAGMENTOVERLAY_H
