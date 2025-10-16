#include "FragmentRegionsOverlay.h"
#include "../core/ProjectModel.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <cmath>

// ========================================================================================================
// ESTRUTURA DO OVERLAY DE FRAGMENTOS
// ========================================================================================================
// 
// OBJETOS PRINCIPAIS:
// 1. currentImage->originalImage    : cv::Mat - Imagem ORIGINAL (sem rotações, tamanho original)
// 2. currentImage->workingImage     : cv::Mat - Imagem ATUAL (pode estar rotacionada e com borda branca)
// 3. FragmentRegionsOverlay (this)  : QWidget - Camada transparente SOBRE o ImageViewer onde desenha retângulos
// 4. fragment->sourceRect           : QRect - Coordenadas do fragmento na imagem ORIGINAL (nunca muda)
// 5. currentImage->currentRotationAngle : double - Ângulo acumulado da imagem (0°, 90°, 180°, 270°, ou arbitrário)
//
// FLUXO DE DESENHO:
// 1. paintEvent() é chamado quando precisa redesenhar
// 2. Para cada fragmento, pega sourceRect (coords originais) e currentRotationAngle
// 3. Calcula onde o retângulo aparece na workingImage rotacionada atual
// 4. Aplica scaleFactor e offsets (zoom e scroll) do ImageViewer
// 5. Desenha o polígono/retângulo na tela
//
// TODO: calcular posições dos fragmentos
// PROBLEMA ATUAL: A transformação de coordenadas (original → rotacionada) não está correta para ângulos arbitrários.
// Os retângulos aparecem em posições erradas quando a imagem tem rotação livre (ex: 13°).
//
// MINI ROTEIRO DO CÁLCULO ATUAL (para depuração):
// 1. sourceRect está em coordenadas da imagem ORIGINAL (4000x3000)
// 2. A imagem foi rotacionada com cv::warpAffine() usando matriz do OpenCV
// 3. A rotação adiciona BORDA BRANCA, aumentando o tamanho (ex: 4572x3822 para 13°)
// 4. calculateRotatedPolygon() tenta replicar a MESMA transformação nos 4 cantos do sourceRect
// 5. Usa cv::getRotationMatrix2D() + ajuste de offset + cv::transform()
// 6. ESPERADO: Os cantos transformados devem coincidir com onde a imagem transformada ficou
// 7. ATUAL: Há uma discrepância - os retângulos não aparecem nas posições corretas
//
// ========================================================================================================

FragmentRegionsOverlay::FragmentRegionsOverlay(QWidget *parent)
    : QWidget(parent)
    , currentImage(nullptr)
    , scaleFactor(1.0)
    , scrollOffset(0, 0)
    , imageOffset(0, 0)
    , showRegions(false)
    , previewRotationAngle(-1.0)
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

    // ============================================================================================
    // TODO: calcular posições dos fragmentos - INÍCIO DA FUNÇÃO DE DESENHO
    // ============================================================================================
    // Esta função é chamada para cada fragmento para desenhar seu retângulo na tela.
    //
    // ENTRADA:
    // - fragment->sourceRect : Coordenadas na imagem ORIGINAL (ex: [2236,1664 974x1027])
    // - currentImage->currentRotationAngle : Ângulo atual da imagem (ex: 13.0°)
    // - currentImage->originalImage : cv::Mat (4000x3000) - imagem sem rotação
    // - currentImage->workingImage : cv::Mat (4572x3822) - imagem rotacionada com borda branca
    //
    // SAÍDA ESPERADA:
    // - Desenhar retângulo na POSIÇÃO CORRETA sobre a workingImage exibida no viewer
    // ============================================================================================

    // sourceRect está SEMPRE em coordenadas da imagem ORIGINAL
    QRect originalSpaceRect = fragment->sourceRect;
    
    // Usar previewRotationAngle se em modo de preview (rotação interativa), senão usar ângulo da imagem
    double angle = (previewRotationAngle >= 0.0) ? previewRotationAngle : currentImage->currentRotationAngle;
    
    fprintf(stderr, "[OVERLAY] Desenhando fragmento %d: sourceRect=[%d,%d %dx%d], ângulo=%.1f (preview=%s)\n",
            index, originalSpaceRect.x(), originalSpaceRect.y(), 
            originalSpaceRect.width(), originalSpaceRect.height(), angle,
            (previewRotationAngle >= 0.0) ? "SIM" : "NAO");
    
    // ============================================================================================
    // TODO: calcular posições dos fragmentos - TRANSFORMAÇÃO PARA ÂNGULOS ARBITRÁRIOS
    // ============================================================================================
    // Para ângulos arbitrários (não múltiplos de 90°), precisa calcular onde os 4 cantos
    // do sourceRect aparecem na workingImage rotacionada.
    // ============================================================================================
    if (fabs(angle) >= 0.1 && (fabs(fmod(angle, 90.0)) > 0.1)) {
        // Rotação arbitrária - calcular 4 cantos rotacionados
        // IMPORTANTE: Não passar rotatedSize, calcular dentro da função baseado no ângulo
        QPolygonF rotatedPoly = calculateRotatedPolygon(
            originalSpaceRect,
            angle,
            QSize(currentImage->originalImage.cols, currentImage->originalImage.rows),
            QSize(0, 0)  // Ignorado - será calculado dentro da função
        );
        
        // Escalar e aplicar offsets (zoom e scroll do viewer)
        QPolygonF scaledPoly;
        for (const QPointF& pt : rotatedPoly) {
            scaledPoly << QPointF(
                pt.x() * scaleFactor + imageOffset.x() - scrollOffset.x(),
                pt.y() * scaleFactor + imageOffset.y() - scrollOffset.y()
            );
        }
        
        drawPolygonAndLabel(painter, scaledPoly, index);
    } else {
        // Rotação múltiplo de 90° - usar método antigo otimizado
        QRect currentSpaceRect = convertOriginalToCurrentCoords(
            originalSpaceRect,
            angle,
            QSize(currentImage->originalImage.cols, currentImage->originalImage.rows),
            QSize(currentImage->workingImage.cols, currentImage->workingImage.rows)
        );
        
        QRectF scaledRect(
            currentSpaceRect.x() * scaleFactor + imageOffset.x() - scrollOffset.x(),
            currentSpaceRect.y() * scaleFactor + imageOffset.y() - scrollOffset.y(),
            currentSpaceRect.width() * scaleFactor,
            currentSpaceRect.height() * scaleFactor
        );
        
        drawRectAndLabel(painter, scaledRect, index);
    }
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

void FragmentRegionsOverlay::drawPolygonAndLabel(QPainter& painter, const QPolygonF& poly, int index) {
    // Desenhar apenas BORDA do polígono (sem preenchimento)
    QColor borderColor(0, 180, 255, 255);
    painter.setPen(QPen(borderColor, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(poly);
    
    // Desenhar rótulo no primeiro vértice (canto superior esquerdo do retângulo original)
    if (!poly.isEmpty()) {
        drawLabel(painter, poly.first(), index);
    }
}

QPolygonF FragmentRegionsOverlay::calculateRotatedPolygon(const QRect& originalRect, double angleDeg,
                                                            const QSize& originalSize, const QSize& rotatedSize) {
    // ============================================================================================
    // TODO: calcular posições dos fragmentos - TRANSFORMAÇÃO DE COORDENADAS
    // ============================================================================================
    // OBJETIVO: Transformar os 4 cantos do originalRect (em coords da imagem original) para
    //           as coordenadas correspondentes na imagem rotacionada (workingImage).
    //
    // MÉTODO ATUAL (usando cv::transform):
    // 1. Cria matriz de rotação usando cv::getRotationMatrix2D() - IGUAL ao RotationDialog
    // 2. Calcula novo tamanho da imagem rotacionada (para incluir borda branca)
    // 3. Ajusta a translação da matriz para centralizar a imagem rotacionada
    // 4. Aplica cv::transform() nos 4 cantos do retângulo
    //
    // ENTRADA:
    // - originalRect : QRect em coords da imagem ORIGINAL (ex: [2236,1664 974x1027])
    // - angleDeg : Ângulo de rotação em graus (ex: 13.0)
    // - originalSize : Tamanho da imagem ORIGINAL (ex: 4000x3000)
    //
    // SAÍDA:
    // - QPolygonF com 4 pontos nas coords da imagem ROTACIONADA (ex: workingImage 4572x3822)
    //
    // PROBLEMA ATUAL:
    // - Os pontos transformados NÃO coincidem com onde a região realmente aparece na workingImage
    // - Provavelmente há diferença no cálculo da matriz ou do ajuste de offset
    // ============================================================================================
    
    // Criar EXATAMENTE a mesma matriz que RotationDialog usa
    cv::Point2f center(originalSize.width() / 2.0f, originalSize.height() / 2.0f);
    cv::Mat rotMatrix = cv::getRotationMatrix2D(center, -angleDeg, 1.0);
    
    // Calcular novo tamanho (igual ao RotationDialog)
    double abs_cos = fabs(rotMatrix.at<double>(0, 0));
    double abs_sin = fabs(rotMatrix.at<double>(0, 1));
    int new_w = int(originalSize.height() * abs_sin + originalSize.width() * abs_cos);
    int new_h = int(originalSize.height() * abs_cos + originalSize.width() * abs_sin);
    
    // Ajustar translação (igual ao RotationDialog)
    rotMatrix.at<double>(0, 2) += (new_w / 2.0) - center.x;
    rotMatrix.at<double>(1, 2) += (new_h / 2.0) - center.y;
    
    fprintf(stderr, "[OVERLAY] calculateRotatedPolygon: angle=%.1f°, original=%dx%d, rotated=%dx%d\n",
            angleDeg, originalSize.width(), originalSize.height(), new_w, new_h);
    fprintf(stderr, "[OVERLAY]   Matriz: [%.4f %.4f %.1f; %.4f %.4f %.1f]\n",
            rotMatrix.at<double>(0,0), rotMatrix.at<double>(0,1), rotMatrix.at<double>(0,2),
            rotMatrix.at<double>(1,0), rotMatrix.at<double>(1,1), rotMatrix.at<double>(1,2));
    
    // Preparar pontos de entrada (4 cantos do retângulo)
    std::vector<cv::Point2f> srcPoints = {
        cv::Point2f(originalRect.left(), originalRect.top()),      // Top-left
        cv::Point2f(originalRect.right(), originalRect.top()),     // Top-right
        cv::Point2f(originalRect.right(), originalRect.bottom()),  // Bottom-right
        cv::Point2f(originalRect.left(), originalRect.bottom())    // Bottom-left
    };
    
    // Aplicar transformação usando OpenCV (100% idêntico ao que ele faz com a imagem)
    std::vector<cv::Point2f> dstPoints;
    cv::transform(srcPoints, dstPoints, rotMatrix);
    
    // Converter para QPolygonF
    QPolygonF rotatedPoly;
    for (size_t i = 0; i < dstPoints.size(); i++) {
        rotatedPoly << QPointF(dstPoints[i].x, dstPoints[i].y);
        fprintf(stderr, "[OVERLAY]   Canto[%zu]: (%.1f,%.1f) → (%.1f,%.1f)\n",
                i, srcPoints[i].x, srcPoints[i].y, dstPoints[i].x, dstPoints[i].y);
    }
    
    // ============================================================================================
    // FIM DO CÁLCULO - Retorna polígono com 4 cantos transformados
    // ============================================================================================
    
    return rotatedPoly;
}

QRect FragmentRegionsOverlay::convertOriginalToCurrentCoords(const QRect& originalRect, double currentAngle,
                                                              const QSize& originalSize, const QSize& currentSize) {
    // Se não há rotação, retornar direto
    if (fabs(currentAngle) < 0.1) {
        return originalRect;
    }
    
    // Normalizar ângulo para múltiplos de 90°
    int angle90 = static_cast<int>(round(currentAngle / 90.0)) % 4;
    if (angle90 < 0) angle90 += 4;
    
    // Converter de coordenadas originais para rotacionadas
    QRect result;
    
    switch (angle90) {
        case 0: // 0° - sem rotação
            result = originalRect;
            break;
            
        case 1: // 90° CW
            // x_rot = y_orig, y_rot = width_orig - x_orig - w_orig
            result = QRect(
                originalRect.y(),
                originalSize.width() - originalRect.x() - originalRect.width(),
                originalRect.height(),
                originalRect.width()
            );
            break;
            
        case 2: // 180°
            result = QRect(
                originalSize.width() - originalRect.x() - originalRect.width(),
                originalSize.height() - originalRect.y() - originalRect.height(),
                originalRect.width(),
                originalRect.height()
            );
            break;
            
        case 3: // 270° CW (90° CCW)
            result = QRect(
                originalSize.height() - originalRect.y() - originalRect.height(),
                originalRect.x(),
                originalRect.height(),
                originalRect.width()
            );
            break;
    }
    
    return result;
}
