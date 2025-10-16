#include "FragmentRegionsOverlay.h"
#include "../core/ProjectModel.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

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
}

void FragmentRegionsOverlay::setImage(FingerprintEnhancer::FingerprintImage* img) {
    fprintf(stderr, "[FragmentRegionsOverlay] setImage: %p\n", (void*)img);
    if (img) {
        fprintf(stderr, "[FragmentRegionsOverlay] Image has %d fragments\n", img->fragments.size());
    }
    fflush(stderr);
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
    fprintf(stderr, "[FragmentRegionsOverlay] setShowRegions: %s\n", show ? "TRUE" : "FALSE");
    fflush(stderr);
    showRegions = show;
    update();
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
    if (!fragment) return;

    // Obter sourceRect (coordenadas na imagem original)
    QRect sourceRect = fragment->sourceRect;
    
    // Converter para coordenadas do widget (escaladas e com offset)
    QRectF scaledRect(
        sourceRect.x() * scaleFactor + imageOffset.x() - scrollOffset.x(),
        sourceRect.y() * scaleFactor + imageOffset.y() - scrollOffset.y(),
        sourceRect.width() * scaleFactor,
        sourceRect.height() * scaleFactor
    );

    // Cor semi-transparente para o retângulo (Azul ciano)
    QColor rectColor(0, 200, 255, 80);  // Azul ciano semi-transparente
    QColor borderColor(0, 150, 255, 255);  // Azul escuro opaco
    
    // Desenhar retângulo preenchido
    painter.setPen(Qt::NoPen);
    painter.setBrush(rectColor);
    painter.drawRect(scaledRect);
    
    // Desenhar borda
    painter.setPen(QPen(borderColor, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(scaledRect);
    
    // Desenhar rótulo com número do fragmento
    QString label = QString("F%1").arg(index);
    
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(label);
    
    // Posição do rótulo (canto superior esquerdo do retângulo)
    int labelX = scaledRect.left() + 5;
    int labelY = scaledRect.top() + fm.height() + 3;
    
    // Desenhar fundo do rótulo
    QRect labelBg(labelX - 3, labelY - fm.height() - 1, textRect.width() + 6, fm.height() + 4);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 150, 255, 220));  // Azul escuro
    painter.drawRoundedRect(labelBg, 3, 3);
    
    // Desenhar texto
    painter.setPen(Qt::white);
    painter.drawText(labelX, labelY - 2, label);
}
