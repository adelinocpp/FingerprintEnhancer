#include "RulerWidget.h"
#include <QPainter>
#include <QFontMetrics>
#include <cmath>

RulerWidget::RulerWidget(Orientation orientation, Position position, QWidget *parent)
    : QWidget(parent),
      orientation(orientation),
      position(position),
      scale(1.0),
      zoomFactor(1.0),
      scrollOffset(0),
      imageOffset(0),
      visible(true),
      rulerWidth(25),
      backgroundColor(QColor(240, 240, 240)),
      textColor(QColor(50, 50, 50)),
      lineColor(QColor(100, 100, 100))
{
    setMinimumSize(rulerWidth, rulerWidth);
}

RulerWidget::~RulerWidget()
{
}

void RulerWidget::setOrientation(Orientation newOrientation)
{
    orientation = newOrientation;
    updateGeometry();
    update();
}

void RulerWidget::setPosition(Position newPosition)
{
    position = newPosition;
    update();
}

void RulerWidget::setScale(double pixelsPerMM)
{
    scale = pixelsPerMM;
    update();
}

void RulerWidget::setZoomFactor(double factor)
{
    zoomFactor = factor;
    update();
}

void RulerWidget::setScrollOffset(int offset)
{
    scrollOffset = offset;
    update();
}

void RulerWidget::setImageOffset(int offset)
{
    imageOffset = offset;
    update();
}

void RulerWidget::setVisible(bool vis)
{
    visible = vis;
    QWidget::setVisible(vis);
}

void RulerWidget::setBackgroundColor(const QColor& color)
{
    backgroundColor = color;
    update();
}

void RulerWidget::setTextColor(const QColor& color)
{
    textColor = color;
    update();
}

void RulerWidget::setLineColor(const QColor& color)
{
    lineColor = color;
    update();
}

void RulerWidget::setRulerWidth(int width)
{
    rulerWidth = width;
    setMinimumSize(width, width);
    updateGeometry();
    update();
}

QSize RulerWidget::sizeHint() const
{
    if (orientation == HORIZONTAL) {
        return QSize(width(), rulerWidth);
    } else {
        return QSize(rulerWidth, height());
    }
}

QSize RulerWidget::minimumSizeHint() const
{
    if (orientation == HORIZONTAL) {
        return QSize(50, rulerWidth);
    } else {
        return QSize(rulerWidth, 50);
    }
}

void RulerWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    if (!visible || scale <= 0) {
        return;
    }
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);  // Linhas mais nítidas
    
    // Fundo
    painter.fillRect(rect(), backgroundColor);
    
    // Desenhar régua baseada na orientação
    if (orientation == HORIZONTAL) {
        drawHorizontalRuler(painter);
    } else {
        drawVerticalRuler(painter);
    }
    
    // Borda
    painter.setPen(lineColor);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void RulerWidget::drawHorizontalRuler(QPainter& painter)
{
    painter.setPen(lineColor);
    
    int rulerLength = width();
    int tickHeight = rulerWidth;
    
    // Calcular espaçamento dos ticks baseado na escala
    double scaledPixelsPerMM = scale * zoomFactor;
    
    // Determinar intervalo apropriado (1mm, 5mm, 10mm, etc.)
    double mmPerPixel = 1.0 / scaledPixelsPerMM;
    double viewWidthMM = rulerLength * mmPerPixel;
    
    int mainInterval = 10;  // 10mm por padrão
    if (viewWidthMM < 50) {
        mainInterval = 5;
    } else if (viewWidthMM < 20) {
        mainInterval = 1;
    } else if (viewWidthMM > 200) {
        mainInterval = 50;
    }
    
    int subInterval = mainInterval / 5;  // Sub-divisões
    if (subInterval < 1) subInterval = 1;
    
    // Calcular offset inicial baseado no scroll
    int startOffsetPixels = scrollOffset - imageOffset;
    double startOffsetMM = startOffsetPixels * mmPerPixel;
    int startMM = static_cast<int>(std::floor(startOffsetMM / mainInterval)) * mainInterval;
    
    // Desenhar ticks e labels
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    
    for (int mm = startMM; mm * scaledPixelsPerMM < rulerLength + startOffsetPixels + 100; mm += subInterval) {
        int pixelPos = static_cast<int>((mm * scaledPixelsPerMM) + imageOffset - scrollOffset);
        
        if (pixelPos < -50 || pixelPos > rulerLength + 50) continue;
        
        bool isMainTick = (mm % mainInterval == 0);
        int tickLen = isMainTick ? tickHeight - 5 : tickHeight / 2;
        
        // Desenhar tick
        if (position == TOP) {
            painter.drawLine(pixelPos, tickHeight - tickLen, pixelPos, tickHeight);
        } else {
            painter.drawLine(pixelPos, 0, pixelPos, tickLen);
        }
        
        // Desenhar label nos ticks principais
        if (isMainTick && mm >= 0) {
            QString label = QString::number(mm);
            drawLabel(painter, pixelPos, label, true);
        }
    }
}

void RulerWidget::drawVerticalRuler(QPainter& painter)
{
    painter.setPen(lineColor);
    
    int rulerLength = height();
    int tickWidth = rulerWidth;
    
    // Calcular espaçamento dos ticks baseado na escala
    double scaledPixelsPerMM = scale * zoomFactor;
    
    // Determinar intervalo apropriado
    double mmPerPixel = 1.0 / scaledPixelsPerMM;
    double viewHeightMM = rulerLength * mmPerPixel;
    
    int mainInterval = 10;
    if (viewHeightMM < 50) {
        mainInterval = 5;
    } else if (viewHeightMM < 20) {
        mainInterval = 1;
    } else if (viewHeightMM > 200) {
        mainInterval = 50;
    }
    
    int subInterval = mainInterval / 5;
    if (subInterval < 1) subInterval = 1;
    
    // Calcular offset inicial
    int startOffsetPixels = scrollOffset - imageOffset;
    double startOffsetMM = startOffsetPixels * mmPerPixel;
    int startMM = static_cast<int>(std::floor(startOffsetMM / mainInterval)) * mainInterval;
    
    // Desenhar ticks e labels
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    
    for (int mm = startMM; mm * scaledPixelsPerMM < rulerLength + startOffsetPixels + 100; mm += subInterval) {
        int pixelPos = static_cast<int>((mm * scaledPixelsPerMM) + imageOffset - scrollOffset);
        
        if (pixelPos < -50 || pixelPos > rulerLength + 50) continue;
        
        bool isMainTick = (mm % mainInterval == 0);
        int tickLen = isMainTick ? tickWidth - 5 : tickWidth / 2;
        
        // Desenhar tick
        if (position == LEFT) {
            painter.drawLine(tickWidth - tickLen, pixelPos, tickWidth, pixelPos);
        } else {
            painter.drawLine(0, pixelPos, tickLen, pixelPos);
        }
        
        // Desenhar label nos ticks principais
        if (isMainTick && mm >= 0) {
            QString label = QString::number(mm);
            drawLabel(painter, pixelPos, label, false);
        }
    }
}

void RulerWidget::drawLabel(QPainter& painter, int pos, const QString& label, bool isHorizontal)
{
    painter.setPen(textColor);
    QFontMetrics fm(painter.font());
    
    if (isHorizontal) {
        QRect textRect = fm.boundingRect(label);
        int x = pos - textRect.width() / 2;
        int y = (position == TOP) ? 3 + fm.ascent() : rulerWidth - 3 - fm.descent();
        painter.drawText(x, y, label);
    } else {
        // Texto vertical (rotacionado)
        painter.save();
        painter.translate(0, pos);
        painter.rotate(-90);
        
        QRect textRect = fm.boundingRect(label);
        int x = -textRect.width() / 2;
        int y = (position == LEFT) ? rulerWidth - 3 : 3 + fm.ascent();
        painter.drawText(x, y, label);
        
        painter.restore();
    }
}

int RulerWidget::calculateTickSpacing() const
{
    // Calcular espaçamento ideal baseado na escala e zoom
    double scaledPixelsPerMM = scale * zoomFactor;
    
    // Queremos ~50-100 pixels entre marcações principais
    int desiredSpacing = 75;
    double mmPerTick = desiredSpacing / scaledPixelsPerMM;
    
    // Arredondar para 1, 2, 5, 10, 20, 50, 100, etc.
    int magnitude = static_cast<int>(std::pow(10, std::floor(std::log10(mmPerTick))));
    int normalized = static_cast<int>(mmPerTick / magnitude);
    
    if (normalized < 2) return magnitude;
    if (normalized < 5) return 2 * magnitude;
    if (normalized < 10) return 5 * magnitude;
    return 10 * magnitude;
}

QString RulerWidget::formatDistance(double mm) const
{
    if (mm < 1.0) {
        return QString("%1 μm").arg(static_cast<int>(mm * 1000));
    } else if (mm < 10.0) {
        return QString("%1 mm").arg(mm, 0, 'f', 1);
    } else {
        return QString("%1 mm").arg(static_cast<int>(mm));
    }
}
