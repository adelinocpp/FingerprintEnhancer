#include "ScaleCalibrationTool.h"
#include <QPainter>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QApplication>
#include <cmath>

ScaleCalibrationTool::ScaleCalibrationTool(QWidget *parent)
    : QWidget(parent),
      active(false),
      currentState(STATE_IDLE),
      zoomFactor(1.0),
      isDrawing(false),
      pixelDistance(0.0),
      ridgeCount(0),
      defaultRidgeSpacing(0.4545)  // 0.4545mm é a distância típica entre cristas
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);  // Não ser transparente para mouse
}

ScaleCalibrationTool::~ScaleCalibrationTool()
{
}

void ScaleCalibrationTool::activate()
{
    active = true;
    currentState = STATE_IDLE;
    reset();
    setVisible(true);
    setFocus();
    emit calibrationStarted();
    emit stateChanged(currentState);
    update();
}

void ScaleCalibrationTool::deactivate()
{
    active = false;
    currentState = STATE_IDLE;
    setVisible(false);
    update();
}

void ScaleCalibrationTool::reset()
{
    lineStart = QPoint();
    lineEnd = QPoint();
    currentPos = QPoint();
    isDrawing = false;
    pixelDistance = 0.0;
    ridgeCount = 0;
    ridgeMarkers.clear();
    currentState = STATE_IDLE;
    emit stateChanged(currentState);
    update();
}

void ScaleCalibrationTool::setImage(const QImage& image)
{
    backgroundImage = image;
    update();
}

void ScaleCalibrationTool::setImageSize(const QSize& size)
{
    imageSize = size;
    update();
}

void ScaleCalibrationTool::setZoomFactor(double factor)
{
    zoomFactor = factor;
    update();
}

void ScaleCalibrationTool::setScrollOffset(const QPoint& offset)
{
    scrollOffset = offset;
    update();
}

void ScaleCalibrationTool::setImageOffset(const QPoint& offset)
{
    imageOffset = offset;
    update();
}

double ScaleCalibrationTool::getCalculatedScale() const
{
    if (ridgeCount < 2 || pixelDistance <= 0) {
        return 0.0;
    }
    
    // Distância real = (número de cristas - 1) * espaçamento médio
    double realDistanceMM = (ridgeCount - 1) * defaultRidgeSpacing;
    
    // Escala = pixels / mm
    return pixelDistance / realDistanceMM;
}

bool ScaleCalibrationTool::hasValidCalibration() const
{
    return ridgeCount >= 2 && pixelDistance > 0 && currentState == STATE_COMPLETED;
}

void ScaleCalibrationTool::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    if (!active) return;
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Desenhar linha de medição
    if (isDrawing || currentState >= STATE_COUNTING_RIDGES) {
        drawCalibrationLine(painter);
    }
    
    // Desenhar marcadores de cristas
    if (currentState >= STATE_COUNTING_RIDGES) {
        drawRidgeMarkers(painter);
    }
    
    // Desenhar instruções
    drawInstructions(painter);
    
    // Desenhar painel de resultados
    if (currentState == STATE_COMPLETED) {
        drawResultsPanel(painter);
    }
}

void ScaleCalibrationTool::wheelEvent(QWheelEvent *event)
{
    // SEMPRE passar eventos de scroll do mouse para o parent (ImageViewer)
    // para permitir zoom durante calibração
    if (parentWidget()) {
        QApplication::sendEvent(parentWidget(), event);
    }
    event->accept();
}

void ScaleCalibrationTool::mousePressEvent(QMouseEvent *event)
{
    // Permitir scroll com botão do meio ou Ctrl+Left
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier)) {
        // Passar para o parent para permitir pan/scroll
        if (parentWidget()) {
            QApplication::sendEvent(parentWidget(), event);
        }
        return;
    }
    
    if (!active || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    
    QPoint imagePos = widgetToImage(event->pos());
    
    if (currentState == STATE_IDLE) {
        // Iniciar desenho da linha
        lineStart = imagePos;
        lineEnd = imagePos;
        currentPos = event->pos();
        isDrawing = true;
        currentState = STATE_DRAWING_LINE;
        emit stateChanged(currentState);
        update();
    }
    else if (currentState == STATE_COUNTING_RIDGES) {
        // Adicionar marcador de crista
        ridgeMarkers.append(imagePos);
        ridgeCount = ridgeMarkers.size();
        emit ridgeCountChanged(ridgeCount);
        
        // Não finalizar automaticamente - usuário deve pressionar Enter
        // Apenas atualizar a visualização
        update();
    }
}

void ScaleCalibrationTool::mouseMoveEvent(QMouseEvent *event)
{
    // Se estiver com botão do meio pressionado, passar para parent (pan)
    if (event->buttons() & Qt::MiddleButton || 
        ((event->buttons() & Qt::LeftButton) && (event->modifiers() & Qt::ControlModifier))) {
        if (parentWidget()) {
            QApplication::sendEvent(parentWidget(), event);
        }
        return;
    }
    
    if (!active) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    
    currentPos = event->pos();
    
    if (isDrawing && currentState == STATE_DRAWING_LINE) {
        lineEnd = widgetToImage(currentPos);
        update();
    }
}

void ScaleCalibrationTool::mouseReleaseEvent(QMouseEvent *event)
{
    // Passar eventos de botão do meio para o parent
    if (event->button() == Qt::MiddleButton) {
        if (parentWidget()) {
            QApplication::sendEvent(parentWidget(), event);
        }
        return;
    }
    
    if (!active || event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    
    if (isDrawing && currentState == STATE_DRAWING_LINE) {
        lineEnd = widgetToImage(event->pos());
        isDrawing = false;
        
        // Calcular distância em pixels
        pixelDistance = calculateDistance(lineStart, lineEnd);
        
        if (pixelDistance > 10.0) {  // Mínimo 10 pixels
            // Avançar para contagem de cristas
            currentState = STATE_COUNTING_RIDGES;
            emit lineDrawn(lineStart, lineEnd, pixelDistance);
            emit stateChanged(currentState);
            
            // Tentar detectar cristas automaticamente
            autoDetectRidges();
        } else {
            // Linha muito curta, resetar
            reset();
        }
        
        update();
    }
}

void ScaleCalibrationTool::keyPressEvent(QKeyEvent *event)
{
    if (!active) {
        QWidget::keyPressEvent(event);
        return;
    }
    
    switch (event->key()) {
        case Qt::Key_Escape:
            reset();
            emit calibrationCancelled();
            break;
            
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
            // Remover último marcador de crista
            if (!ridgeMarkers.isEmpty()) {
                ridgeMarkers.removeLast();
                ridgeCount = ridgeMarkers.size();
                emit ridgeCountChanged(ridgeCount);
                
                if (ridgeCount < 2) {
                    currentState = STATE_COUNTING_RIDGES;
                    emit stateChanged(currentState);
                }
                update();
            }
            break;
            
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // Finalizar calibração
            if (ridgeCount >= 2 && pixelDistance > 0) {
                currentState = STATE_COMPLETED;
                double scale = getCalculatedScale();
                double confidence = estimateConfidence();
                emit calibrationCompleted(scale, confidence);
                emit stateChanged(currentState);
                update();
            }
            break;
            
        default:
            QWidget::keyPressEvent(event);
    }
}

QPoint ScaleCalibrationTool::imageToWidget(const QPoint& imagePoint) const
{
    QPoint scaledPoint(
        static_cast<int>(imagePoint.x() * zoomFactor),
        static_cast<int>(imagePoint.y() * zoomFactor)
    );
    
    return scaledPoint + imageOffset - scrollOffset;
}

QPoint ScaleCalibrationTool::widgetToImage(const QPoint& widgetPoint) const
{
    QPoint imagePoint = widgetPoint + scrollOffset - imageOffset;
    
    return QPoint(
        static_cast<int>(imagePoint.x() / zoomFactor),
        static_cast<int>(imagePoint.y() / zoomFactor)
    );
}

double ScaleCalibrationTool::calculateDistance(const QPoint& p1, const QPoint& p2) const
{
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    return std::sqrt(dx * dx + dy * dy);
}

void ScaleCalibrationTool::drawCalibrationLine(QPainter& painter)
{
    QPoint widgetStart = imageToWidget(lineStart);
    QPoint widgetEnd = isDrawing ? currentPos : imageToWidget(lineEnd);
    
    // Linha principal
    QPen linePen(QColor(0, 255, 0, 200), 2, Qt::SolidLine);
    painter.setPen(linePen);
    painter.drawLine(widgetStart, widgetEnd);
    
    // Círculos nas extremidades
    painter.setBrush(QColor(0, 255, 0, 150));
    painter.drawEllipse(widgetStart, 5, 5);
    painter.drawEllipse(widgetEnd, 5, 5);
    
    // Desenhar cotas perpendiculares para orientação
    // Cotas a cada 20% do comprimento da linha
    QPen quotaPen(QColor(0, 255, 0, 150), 1, Qt::SolidLine);
    painter.setPen(quotaPen);
    
    QPoint lineVec = widgetEnd - widgetStart;
    double lineLength = std::sqrt(lineVec.x() * lineVec.x() + lineVec.y() * lineVec.y());
    
    if (lineLength > 0) {
        // Vetor perpendicular normalizado
        QPoint perpVec(-lineVec.y(), lineVec.x());
        perpVec = QPoint(perpVec.x() / lineLength * 10, perpVec.y() / lineLength * 10);
        
        // Desenhar 5 cotas perpendiculares
        for (int i = 1; i <= 4; ++i) {
            double t = i / 5.0;
            QPoint pos = widgetStart + QPoint(lineVec.x() * t, lineVec.y() * t);
            painter.drawLine(pos - perpVec, pos + perpVec);
        }
    }
    
    // Mostrar distância em pixels (deslocada ABAIXO da linha)
    QPoint midPoint = (widgetStart + widgetEnd) / 2;

    // Calcular vetor perpendicular para deslocar a label ABAIXO
    if (lineLength > 0) {
        // Vetor perpendicular normalizado (para baixo)
        QPoint perpVec(-lineVec.y(), lineVec.x());
        perpVec = QPoint(perpVec.x() / lineLength * 40, perpVec.y() / lineLength * 40);
        midPoint += perpVec;  // Deslocar 40 pixels perpendicular à linha
    }
    
    QString distText = QString("%1 px").arg(pixelDistance, 0, 'f', 1);
    
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(distText);
    textRect.moveCenter(midPoint);
    textRect.adjust(-5, -5, 5, 5);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.drawRect(textRect);
    
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, distText);
}

void ScaleCalibrationTool::drawRidgeMarkers(QPainter& painter)
{
    painter.setBrush(QColor(255, 255, 0, 200));
    painter.setPen(QPen(QColor(255, 0, 0, 255), 2));
    
    for (int i = 0; i < ridgeMarkers.size(); ++i) {
        QPoint widgetPos = imageToWidget(ridgeMarkers[i]);
        
        // Círculo numerado
        painter.drawEllipse(widgetPos, 6, 6);
        
        // Número da crista
        QFont font = painter.font();
        font.setPointSize(9);
        font.setBold(true);
        painter.setFont(font);
        
        painter.setPen(Qt::red);
        painter.drawText(widgetPos + QPoint(10, 5), QString::number(i + 1));
    }
}

void ScaleCalibrationTool::drawInstructions(QPainter& painter)
{
    QFont font = painter.font();
    font.setPointSize(11);
    font.setBold(true);
    painter.setFont(font);
    
    QString instruction;
    QColor bgColor;
    
    switch (currentState) {
        case STATE_IDLE:
        case STATE_DRAWING_LINE:
            instruction = "1. Desenhe uma linha através das cristas perpendiculares";
            bgColor = QColor(0, 100, 200, 200);
            break;
            
        case STATE_COUNTING_RIDGES:
            instruction = QString("2. Clique em cada crista (%1 marcadas) [Enter=Concluir, Del=Remover]")
                         .arg(ridgeCount);
            bgColor = QColor(200, 100, 0, 200);
            break;
            
        case STATE_COMPLETED:
            instruction = QString("✓ Calibração concluída: %1 cristas, %2 px/mm")
                         .arg(ridgeCount)
                         .arg(getCalculatedScale(), 0, 'f', 2);
            bgColor = QColor(0, 150, 0, 200);
            break;
    }
    
    // Painel de instruções no topo
    QRect instrRect(10, 10, width() - 20, 40);
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(instrRect, 5, 5);
    
    painter.setPen(Qt::white);
    painter.drawText(instrRect, Qt::AlignCenter, instruction);
    
    // Informação adicional
    if (currentState == STATE_COUNTING_RIDGES || currentState == STATE_COMPLETED) {
        font.setPointSize(9);
        font.setBold(false);
        painter.setFont(font);
        
        QString info = QString("Distância típica entre cristas: %1 mm").arg(defaultRidgeSpacing, 0, 'f', 4);
        
        QRect infoRect(10, 55, width() - 20, 25);
        painter.setBrush(QColor(0, 0, 0, 180));
        painter.drawRoundedRect(infoRect, 3, 3);
        painter.drawText(infoRect, Qt::AlignCenter, info);
    }
}

void ScaleCalibrationTool::drawResultsPanel(QPainter& painter)
{
    if (ridgeCount < 2 || pixelDistance <= 0) return;
    
    // Painel de resultados no canto inferior direito
    int panelWidth = 300;
    int panelHeight = 150;
    QRect panelRect(width() - panelWidth - 10, height() - panelHeight - 10,
                    panelWidth, panelHeight);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 220));
    painter.drawRoundedRect(panelRect, 8, 8);
    
    // Borda
    painter.setPen(QPen(QColor(0, 200, 0), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(panelRect, 8, 8);
    
    // Título
    QFont titleFont = painter.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(0, 255, 0));
    
    QRect titleRect = panelRect.adjusted(10, 10, -10, -120);
    painter.drawText(titleRect, Qt::AlignLeft, "📏 Resultados da Calibração");
    
    // Dados
    QFont dataFont = painter.font();
    dataFont.setPointSize(10);
    dataFont.setBold(false);
    painter.setFont(dataFont);
    painter.setPen(Qt::white);
    
    int yPos = panelRect.top() + 35;
    int lineHeight = 22;
    
    double realDistanceMM = (ridgeCount - 1) * defaultRidgeSpacing;
    double scale = getCalculatedScale();
    double confidence = estimateConfidence();
    double dpi = scale * 25.4;  // 1 inch = 25.4 mm
    
    QStringList results;
    results << QString("Cristas marcadas: %1").arg(ridgeCount);
    results << QString("Distância em pixels: %1").arg(pixelDistance, 0, 'f', 1);
    results << QString("Distância real: %1 mm").arg(realDistanceMM, 0, 'f', 2);
    results << QString("Escala: %1 px/mm").arg(scale, 0, 'f', 2);
    results << QString("Resolução: %1 DPI").arg(dpi, 0, 'f', 0);
    results << QString("Confiança: %1%").arg(confidence, 0, 'f', 0);
    
    for (const QString& line : results) {
        painter.drawText(panelRect.left() + 15, yPos, line);
        yPos += lineHeight;
    }
}

void ScaleCalibrationTool::autoDetectRidges()
{
    // TODO: Implementar detecção automática de cristas ao longo da linha
    // Por enquanto, deixar o usuário marcar manualmente
    
    // Algoritmo sugerido:
    // 1. Extrair perfil de intensidade ao longo da linha
    // 2. Aplicar filtro passa-baixa
    // 3. Detectar picos (vales) no perfil
    // 4. Marcar posições automaticamente
}

double ScaleCalibrationTool::estimateConfidence() const
{
    if (ridgeCount < 2) return 0.0;
    
    // Fatores que afetam confiança:
    // - Número de cristas (mais = melhor)
    // - Comprimento da linha
    // - Regularidade do espaçamento
    
    double confidence = 50.0;  // Base
    
    // Bônus por número de cristas
    confidence += std::min(30.0, (ridgeCount - 2) * 5.0);
    
    // Bônus por comprimento de linha adequado
    if (pixelDistance > 50) {
        confidence += std::min(20.0, pixelDistance / 10.0);
    }
    
    return std::min(100.0, confidence);
}

void ScaleCalibrationTool::contextMenuEvent(QContextMenuEvent *event)
{
    if (!active) {
        QWidget::contextMenuEvent(event);
        return;
    }
    
    // Menu apenas no estado de contagem de cristas
    if (currentState != STATE_COUNTING_RIDGES && currentState != STATE_COMPLETED) {
        event->accept();
        return;
    }
    
    // Verificar se clicou próximo a algum marcador
    QPoint widgetPos = event->pos();
    int nearestIndex = findNearestRidgeMarker(widgetPos, 15.0);
    
    QMenu menu(this);
    
    if (nearestIndex >= 0) {
        // Clicou próximo a um marcador - opção de remover
        QAction *removeAction = menu.addAction(QString("🗑️ Remover Crista #%1").arg(nearestIndex + 1));
        connect(removeAction, &QAction::triggered, [this, nearestIndex]() {
            removeRidgeMarker(nearestIndex);
        });
    }
    
    if (ridgeCount > 0) {
        // Opção de remover última crista
        QAction *removeLastAction = menu.addAction("↶ Remover Última Crista");
        connect(removeLastAction, &QAction::triggered, [this]() {
            if (!ridgeMarkers.isEmpty()) {
                ridgeMarkers.removeLast();
                ridgeCount = ridgeMarkers.size();
                emit ridgeCountChanged(ridgeCount);
                update();
            }
        });
        
        menu.addSeparator();
        
        // Opção de limpar todas as cristas
        QAction *clearAllAction = menu.addAction("🗑️ Limpar Todas as Cristas");
        connect(clearAllAction, &QAction::triggered, [this]() {
            ridgeMarkers.clear();
            ridgeCount = 0;
            emit ridgeCountChanged(ridgeCount);
            update();
        });
    }
    
    if (!menu.isEmpty()) {
        menu.exec(event->globalPos());
    }
    
    event->accept();
}

int ScaleCalibrationTool::findNearestRidgeMarker(const QPoint& pos, double maxDistance) const
{
    if (ridgeMarkers.isEmpty()) return -1;
    
    int nearestIndex = -1;
    double minDistance = maxDistance;
    
    for (int i = 0; i < ridgeMarkers.size(); ++i) {
        QPoint widgetPos = imageToWidget(ridgeMarkers[i]);
        double dist = calculateDistance(widgetPos, pos);
        
        if (dist < minDistance) {
            minDistance = dist;
            nearestIndex = i;
        }
    }
    
    return nearestIndex;
}

void ScaleCalibrationTool::removeRidgeMarker(int index)
{
    if (index >= 0 && index < ridgeMarkers.size()) {
        ridgeMarkers.removeAt(index);
        ridgeCount = ridgeMarkers.size();
        emit ridgeCountChanged(ridgeCount);
        update();
    }
}
