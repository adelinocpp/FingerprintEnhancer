#include "FragmentOverlay.h"
#include <QMouseEvent>
#include <QPainterPath>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FragmentOverlay::FragmentOverlay(QWidget *parent)
    : QWidget(parent),
      currentImageRotation(0.0),
      fragmentsVisible(true),
      zoomFactor(1.0),
      scrollOffset(0, 0),
      imageOffset(0, 0),
      imageSize(0, 0)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
}

FragmentOverlay::~FragmentOverlay()
{
}

void FragmentOverlay::setFragments(const QVector<FingerprintEnhancer::Fragment>& frags)
{
    fragments = frags;
    update();
}

void FragmentOverlay::setCurrentImageRotation(double angle)
{
    currentImageRotation = angle;
    update();
}

void FragmentOverlay::setZoomFactor(double factor)
{
    zoomFactor = factor;
    update();
}

void FragmentOverlay::setScrollOffset(const QPoint& offset)
{
    scrollOffset = offset;
    update();
}

void FragmentOverlay::setImageOffset(const QPoint& offset)
{
    imageOffset = offset;
    update();
}

void FragmentOverlay::setImageSize(const QSize& size)
{
    imageSize = size;
    update();
}

void FragmentOverlay::setFragmentsVisible(bool visible)
{
    fragmentsVisible = visible;
    update();
}

void FragmentOverlay::setSelectedFragmentId(const QString& fragmentId)
{
    selectedFragmentId = fragmentId;
    update();
}

void FragmentOverlay::setHighlightedFragmentId(const QString& fragmentId)
{
    if (highlightedFragmentId != fragmentId) {
        highlightedFragmentId = fragmentId;
        update();
    }
}

void FragmentOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    if (!fragmentsVisible || fragments.isEmpty()) {
        return;
    }
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Desenhar cada fragmento
    for (const auto& fragment : fragments) {
        drawFragment(painter, fragment);
    }
}

void FragmentOverlay::drawFragment(QPainter& painter, const FingerprintEnhancer::Fragment& fragment)
{
    // Obter polígono do fragmento (retângulo rotacionado)
    QPolygonF polygon = getFragmentPolygon(fragment);
    
    if (polygon.isEmpty()) {
        return;
    }
    
    // Determinar cor e estilo baseado no estado
    bool isSelected = (fragment.id == selectedFragmentId);
    bool isHighlighted = (fragment.id == highlightedFragmentId);
    
    QColor color;
    int penWidth;
    
    if (isSelected) {
        color = QColor(255, 200, 0);  // Laranja para selecionado
        penWidth = 3;
    } else if (isHighlighted) {
        color = QColor(0, 255, 255);  // Ciano para hover
        penWidth = 2;
    } else {
        color = QColor(255, 0, 255);  // Magenta para fragmentos normais
        penWidth = 2;
    }
    
    // Desenhar retângulo com transparência
    QPen pen(color, penWidth, Qt::SolidLine);
    painter.setPen(pen);
    
    QColor fillColor = color;
    fillColor.setAlpha(30);  // Semi-transparente
    painter.setBrush(fillColor);
    
    painter.drawPolygon(polygon);
    
    // Desenhar cantos (círculos nos vértices)
    painter.setBrush(color);
    for (const QPointF& point : polygon) {
        painter.drawEllipse(point, 3, 3);
    }
    
    // Desenhar label
    drawFragmentLabel(painter, fragment, polygon);
}

void FragmentOverlay::drawFragmentLabel(QPainter& painter, 
                                       const FingerprintEnhancer::Fragment& fragment,
                                       const QPolygonF& polygon)
{
    if (polygon.size() < 3) return;
    
    // Calcular centro do polígono
    QPointF center(0, 0);
    for (const QPointF& point : polygon) {
        center += point;
    }
    center /= polygon.size();
    
    // Texto do label
    QString label = QString("F%1").arg(fragment.id.left(4));
    if (fragment.minutiae.size() > 0) {
        label += QString(" (%1)").arg(fragment.minutiae.size());
    }
    
    // Configurar fonte
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(label);
    textRect.moveCenter(center.toPoint());
    textRect.adjust(-4, -2, 4, 2);
    
    // Desenhar fundo do texto
    painter.setPen(Qt::NoPen);
    QColor bgColor(0, 0, 0, 180);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(textRect, 3, 3);
    
    // Desenhar texto
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, label);
}

QPolygonF FragmentOverlay::getFragmentPolygon(const FingerprintEnhancer::Fragment& fragment) const
{
    if (imageSize.isEmpty()) {
        return QPolygonF();
    }
    
    // Centro da imagem (ponto de rotação)
    QPointF imageCenter(imageSize.width() / 2.0, imageSize.height() / 2.0);
    
    // Diferença de ângulo entre quando fragmento foi criado e agora
    double angleDiff = currentImageRotation - fragment.sourceRotationAngle;
    
    // Os 4 cantos do retângulo original
    QRectF sourceRect = fragment.sourceRect;
    QVector<QPointF> corners;
    corners << sourceRect.topLeft()
            << sourceRect.topRight()
            << sourceRect.bottomRight()
            << sourceRect.bottomLeft();
    
    // Rotacionar cada canto
    QPolygonF polygon;
    for (const QPointF& corner : corners) {
        QPointF rotated = rotatePoint(corner, imageCenter, angleDiff);
        QPoint widget = imageToWidget(rotated);
        polygon << widget;
    }
    
    return polygon;
}

QPointF FragmentOverlay::rotatePoint(const QPointF& point, const QPointF& center, double angleDeg) const
{
    double angleRad = angleDeg * M_PI / 180.0;
    
    // Transladar para origem
    QPointF translated = point - center;
    
    // Rotacionar
    double cosA = std::cos(angleRad);
    double sinA = std::sin(angleRad);
    
    QPointF rotated(
        translated.x() * cosA - translated.y() * sinA,
        translated.x() * sinA + translated.y() * cosA
    );
    
    // Transladar de volta
    return rotated + center;
}

QPoint FragmentOverlay::imageToWidget(const QPointF& imagePoint) const
{
    QPoint scaledPoint(
        static_cast<int>(imagePoint.x() * zoomFactor),
        static_cast<int>(imagePoint.y() * zoomFactor)
    );
    
    return scaledPoint + imageOffset - scrollOffset;
}

QPointF FragmentOverlay::widgetToImage(const QPoint& widgetPoint) const
{
    QPoint imagePoint = widgetPoint + scrollOffset - imageOffset;
    
    return QPointF(
        imagePoint.x() / zoomFactor,
        imagePoint.y() / zoomFactor
    );
}

QRectF FragmentOverlay::getImageBounds() const
{
    return QRectF(0, 0, imageSize.width(), imageSize.height());
}

QString FragmentOverlay::findFragmentAtPosition(const QPoint& widgetPos) const
{
    // Procurar de trás para frente (fragmentos mais recentes primeiro)
    for (int i = fragments.size() - 1; i >= 0; --i) {
        const auto& fragment = fragments[i];
        QPolygonF polygon = getFragmentPolygon(fragment);
        
        if (polygon.containsPoint(widgetPos, Qt::OddEvenFill)) {
            return fragment.id;
        }
    }
    
    return QString();
}

void FragmentOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && fragmentsVisible) {
        QString fragmentId = findFragmentAtPosition(event->pos());
        
        if (!fragmentId.isEmpty()) {
            QPointF imagePos = widgetToImage(event->pos());
            emit fragmentClicked(fragmentId, imagePos.toPoint());
            event->accept();
            return;
        }
    }
    
    QWidget::mousePressEvent(event);
}

void FragmentOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (fragmentsVisible) {
        QString fragmentId = findFragmentAtPosition(event->pos());
        
        if (fragmentId != highlightedFragmentId) {
            setHighlightedFragmentId(fragmentId);
            
            if (!fragmentId.isEmpty()) {
                emit fragmentHovered(fragmentId);
            }
        }
    }
    
    QWidget::mouseMoveEvent(event);
}

void FragmentOverlay::leaveEvent(QEvent *event)
{
    setHighlightedFragmentId(QString());
    QWidget::leaveEvent(event);
}
