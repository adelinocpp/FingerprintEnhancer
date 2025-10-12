#include "MinutiaeOverlay.h"
#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QEvent>
#include <QDebug>
#include <cmath>

namespace FingerprintEnhancer {

MinutiaeOverlay::MinutiaeOverlay(QWidget *parent)
    : QWidget(parent),
      currentFragment(nullptr),
      scaleFactor(1.0),
      scrollOffset(0, 0),
      imageOffset(0, 0),
      editMode(false),
      editState(MinutiaEditState::IDLE),
      isDragging(false),
      initialAngle(0.0f),
      showLabels(true),
      showAngles(false),
      minutiaRadius(10),
      normalColor(255, 0, 0),      // Vermelho
      selectedColor(0, 255, 0),     // Verde
      hoverColor(255, 255, 0)       // Amarelo
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMouseTracking(true);
}

void MinutiaeOverlay::setFragment(Fragment* fragment) {
    currentFragment = fragment;
    clearSelection();
    update();
}

void MinutiaeOverlay::setScaleFactor(double scale) {
    scaleFactor = scale;
    update();
}

void MinutiaeOverlay::setScrollOffset(const QPoint& offset) {
    scrollOffset = offset;
    update();
}

void MinutiaeOverlay::setImageOffset(const QPoint& offset) {
    imageOffset = offset;
    update();
}

void MinutiaeOverlay::clearMinutiae() {
    currentFragment = nullptr;
    clearSelection();
    update();
}

void MinutiaeOverlay::setSelectedMinutia(const QString& minutiaId) {
    selectedMinutiaId = minutiaId;
    update();
}

void MinutiaeOverlay::clearSelection() {
    selectedMinutiaId.clear();
    editState = MinutiaEditState::IDLE;
    isDragging = false;
    emit editStateChanged(editState);
    update();
}

void MinutiaeOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    if (!currentFragment || currentFragment->minutiae.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Desenhar todas as minúcias
    int number = 1;
    for (const auto& minutia : currentFragment->minutiae) {
        bool isSelected = (minutia.id == selectedMinutiaId);
        QPoint scaledPos = scalePoint(minutia.position);
        
        // Desenhar círculo + seta (sempre visível no modo de edição)
        if (editMode) {
            QColor color = isSelected ? selectedColor : normalColor;
            drawMinutiaWithArrow(painter, scaledPos, minutia.angle, color, isSelected);
        } else {
            drawMinutia(painter, minutia, isSelected);
        }

        if (showLabels) {
            drawMinutiaLabel(painter, scaledPos, number, minutia.getTypeAbbreviation(), isSelected, minutia.labelPosition);
        }

        number++;
    }
    
    // Desenhar indicador de estado de edição
    if (editMode && !selectedMinutiaId.isEmpty()) {
        for (const auto& minutia : currentFragment->minutiae) {
            if (minutia.id == selectedMinutiaId) {
                QPoint scaledPos = scalePoint(minutia.position);
                drawEditStateIndicator(painter, scaledPos);
                break;
            }
        }
    }
}

void MinutiaeOverlay::drawMinutia(QPainter& painter, const Minutia& minutia, bool isSelected) {
    QPoint pos = scalePoint(minutia.position);
    QColor color = isSelected ? selectedColor : normalColor;

    // Desenhar marcador usando o símbolo configurado
    int size = displaySettings.markerSize;
    QPen pen(color, 2);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    switch (displaySettings.symbol) {
        case MinutiaeSymbol::CIRCLE:
            // Círculo simples
            painter.drawEllipse(pos, size/2, size/2);
            break;

        case MinutiaeSymbol::CIRCLE_X:
            // Círculo com X
            painter.drawEllipse(pos, size/2, size/2);
            {
                int offset = size / 4;
                painter.drawLine(pos.x() - offset, pos.y() - offset, pos.x() + offset, pos.y() + offset);
                painter.drawLine(pos.x() - offset, pos.y() + offset, pos.x() + offset, pos.y() - offset);
            }
            break;

        case MinutiaeSymbol::CIRCLE_ARROW:
            // Círculo com seta (ângulo)
            painter.drawEllipse(pos, size/2, size/2);
            {
                double radians = minutia.angle * M_PI / 180.0;
                int arrowLen = 1.5*size / 2;
                int endX = pos.x() + static_cast<int>(arrowLen * cos(radians));
                int endY = pos.y() - static_cast<int>(arrowLen * sin(radians));
                painter.drawLine(pos, QPoint(endX, endY));

                // Pontas da seta
                double arrowSize = size/5;
                double arrowAngle1 = radians + M_PI * 3.0 / 4.0;
                double arrowAngle2 = radians - M_PI * 3.0 / 4.0;
                QPoint arrow1(endX + static_cast<int>(arrowSize * cos(arrowAngle1)),
                              endY - static_cast<int>(arrowSize * sin(arrowAngle1)));
                QPoint arrow2(endX + static_cast<int>(arrowSize * cos(arrowAngle2)),
                              endY - static_cast<int>(arrowSize * sin(arrowAngle2)));
                painter.drawLine(endX, endY, arrow1.x(), arrow1.y());
                painter.drawLine(endX, endY, arrow2.x(), arrow2.y());
            }
            break;

        case MinutiaeSymbol::CIRCLE_CROSS:
            // Círculo com cruz
            painter.drawEllipse(pos, size/2, size/2);
            {
                int offset = size / 4;
                painter.drawLine(pos.x() - offset, pos.y(), pos.x() + offset, pos.y());
                painter.drawLine(pos.x(), pos.y() - offset, pos.x(), pos.y() + offset);
            }
            break;

        case MinutiaeSymbol::TRIANGLE:
            // Triângulo
            {
                int h = size / 2;
                QPolygon triangle;
                triangle << QPoint(pos.x(), pos.y() - h)
                        << QPoint(pos.x() - h, pos.y() + h/2)
                        << QPoint(pos.x() + h, pos.y() + h/2);
                painter.drawPolygon(triangle);
            }
            break;

        case MinutiaeSymbol::SQUARE:
            // Quadrado
            {
                int h = size / 2;
                painter.drawRect(pos.x() - h, pos.y() - h, size, size);
            }
            break;

        case MinutiaeSymbol::DIAMOND:
            // Losango
            {
                int h = size / 2;
                QPolygon diamond;
                diamond << QPoint(pos.x(), pos.y() - h)
                       << QPoint(pos.x() + h, pos.y())
                       << QPoint(pos.x(), pos.y() + h)
                       << QPoint(pos.x() - h, pos.y());
                painter.drawPolygon(diamond);
            }
            break;
    }

    // Desenhar ângulo adicional se habilitado (linha externa)
    if (showAngles && displaySettings.symbol != MinutiaeSymbol::CIRCLE_ARROW) {
        int lineLength = size * 2;
        double radians = minutia.angle * M_PI / 180.0;
        int endX = pos.x() + static_cast<int>(lineLength * cos(radians));
        int endY = pos.y() - static_cast<int>(lineLength * sin(radians));
        painter.drawLine(pos.x(), pos.y(), endX, endY);
    }
}


void MinutiaeOverlay::drawMinutiaLabel(QPainter& painter, const QPoint& pos, int number, const QString& type, bool isSelected, MinutiaLabelPosition labelPos) {
    // Se oculto, não desenhar
    if (labelPos == MinutiaLabelPosition::HIDDEN) {
        return;
    }
    
    QColor textColor = isSelected ? selectedColor : normalColor;
    QFont font = painter.font();
    font.setPointSize(displaySettings.labelFontSize);
    font.setBold(isSelected);
    painter.setFont(font);

    int size = displaySettings.markerSize;
    int margin = 5;

    // Desenhar número com espaços
    QString label = QString(" %1 .").arg(number);
    QRect textRect = painter.fontMetrics().boundingRect(label);
    QPoint textPos;
    
    // Calcular posição baseado em labelPos
    switch (labelPos) {
        case MinutiaLabelPosition::RIGHT:
            textPos = QPoint(pos.x() + size/2 + margin, pos.y() - size/2);
            break;
        case MinutiaLabelPosition::LEFT:
            textPos = QPoint(pos.x() - size/2 - margin - textRect.width(), pos.y() - size/2);
            break;
        case MinutiaLabelPosition::ABOVE:
            textPos = QPoint(pos.x() - textRect.width()/2, pos.y() - size/2 - margin - textRect.height());
            break;
        case MinutiaLabelPosition::BELOW:
            textPos = QPoint(pos.x() - textRect.width()/2, pos.y() + size/2 + margin + textRect.height());
            break;
        default:
            textPos = QPoint(pos.x() + size/2 + margin, pos.y() - size/2);
    }

    // Fundo configurável para melhor legibilidade
    painter.fillRect(textRect.translated(textPos), displaySettings.labelBackgroundColor);
    painter.setPen(textColor);
    painter.drawText(textPos, label);

    // Desenhar tipo (abreviação) com espaços
    if (!type.isEmpty() && type != "N/C") {
        QString typeLabel = QString(" %1 .").arg(type);
        QRect typeRect = painter.fontMetrics().boundingRect(typeLabel);
        QPoint typePos;
        
        // Tipo sempre abaixo do número
        switch (labelPos) {
            case MinutiaLabelPosition::RIGHT:
                typePos = QPoint(pos.x() + size/2 + margin, pos.y() + 5);
                break;
            case MinutiaLabelPosition::LEFT:
                typePos = QPoint(pos.x() - size/2 - margin - typeRect.width(), pos.y() + 5);
                break;
            case MinutiaLabelPosition::ABOVE:
                typePos = QPoint(pos.x() - typeRect.width()/2, pos.y() - size/2 - margin - textRect.height() - typeRect.height() - 2);
                break;
            case MinutiaLabelPosition::BELOW:
                typePos = QPoint(pos.x() - typeRect.width()/2, pos.y() + size/2 + margin + textRect.height() + typeRect.height() + 2);
                break;
            default:
                typePos = QPoint(pos.x() + size/2 + margin, pos.y() + 5);
        }
        
        painter.fillRect(typeRect.translated(typePos), displaySettings.labelBackgroundColor);
        painter.drawText(typePos, typeLabel);
    }
}

void MinutiaeOverlay::mousePressEvent(QMouseEvent *event) {
    qDebug() << "MinutiaeOverlay::mousePressEvent - button:" << event->button() 
             << "pos:" << event->pos() 
             << "editMode:" << editMode
             << "currentFragment:" << (currentFragment != nullptr);
    
    if (!currentFragment) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        qDebug() << "Left button clicked, searching for minutia...";
        Minutia* minutia = findMinutiaAt(event->pos());
        
        if (minutia) {
            qDebug() << "Minutia found:" << minutia->id.left(8);
            // Se já está selecionada e no modo de edição, alterna entre mover/rotacionar
            if (editMode && minutia->id == selectedMinutiaId && 
                (editState == MinutiaEditState::EDITING_POSITION || editState == MinutiaEditState::EDITING_ANGLE)) {
                // Alternar entre mover e rotacionar
                if (editState == MinutiaEditState::EDITING_POSITION) {
                    editState = MinutiaEditState::EDITING_ANGLE;
                    initialAngle = minutia->angle;
                } else {
                    editState = MinutiaEditState::EDITING_POSITION;
                }
                emit editStateChanged(editState);
            } else {
                // Selecionar minúcia
                selectedMinutiaId = minutia->id;
                if (editMode) {
                    editState = MinutiaEditState::SELECTED;
                } else {
                    editState = MinutiaEditState::IDLE;
                }
                initialAngle = minutia->angle;
            }
            
            isDragging = true;
            dragStartPos = event->pos();
            lastDragPos = event->pos();
            emit minutiaClicked(minutia->id, minutia->position);
            update();
        } else {
            clearSelection();
        }
    }

    QWidget::mousePressEvent(event);
}

void MinutiaeOverlay::mouseDoubleClickEvent(QMouseEvent *event) {
    if (!currentFragment) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        Minutia* minutia = findMinutiaAt(event->pos());
        if (minutia) {
            emit minutiaDoubleClicked(minutia->id);
        }
    }

    QWidget::mouseDoubleClickEvent(event);
}

void MinutiaeOverlay::mouseMoveEvent(QMouseEvent *event) {
    if (!editMode || selectedMinutiaId.isEmpty()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    // Só processa se o botão esquerdo estiver pressionado (arrastando)
    if (event->buttons() & Qt::LeftButton) {
        if (editState == MinutiaEditState::EDITING_POSITION) {
            // Arrastar para mover posição
            QPoint newPos = unscalePoint(event->pos());
            emit positionChanged(selectedMinutiaId, newPos);
            lastDragPos = event->pos();
            update();
            
        } else if (editState == MinutiaEditState::EDITING_ANGLE) {
            // Arrastar para rotacionar ângulo
            Minutia* minutia = nullptr;
            for (auto& m : currentFragment->minutiae) {
                if (m.id == selectedMinutiaId) {
                    minutia = &m;
                    break;
                }
            }
            
            if (minutia) {
                QPoint center = scalePoint(minutia->position);
                float newAngle = calculateAngleFromDrag(center, event->pos());
                emit angleChanged(selectedMinutiaId, newAngle);
                lastDragPos = event->pos();
                update();
            }
        }
    }

    QWidget::mouseMoveEvent(event);
}

void MinutiaeOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }

    QWidget::mouseReleaseEvent(event);
}

void MinutiaeOverlay::contextMenuEvent(QContextMenuEvent *event) {
    if (!currentFragment || !editMode) {
        QWidget::contextMenuEvent(event);
        return;
    }

    // Verificar se clicou em uma minúcia
    Minutia* minutia = findMinutiaAt(event->pos());
    if (!minutia) {
        QWidget::contextMenuEvent(event);
        return;
    }

    // Se não está selecionada, selecionar
    if (minutia->id != selectedMinutiaId) {
        selectedMinutiaId = minutia->id;
        editState = MinutiaEditState::SELECTED;
        initialAngle = minutia->angle;
        emit minutiaClicked(minutia->id, minutia->position);
        update();
    }

    // Criar menu de contexto
    QMenu menu(this);
    menu.setStyleSheet("QMenu { font-size: 11pt; }");
    
    QAction* moveAction = menu.addAction("↔️ Mover Minúcia");
    QAction* rotateAction = menu.addAction("🔄 Rotacionar Minúcia");
    
    menu.addSeparator();
    
    // Submenu de posição do rótulo
    QMenu* labelPosMenu = menu.addMenu("📍 Posição do Rótulo");
    QAction* labelRightAction = labelPosMenu->addAction("→ À Direita");
    QAction* labelLeftAction = labelPosMenu->addAction("← À Esquerda");
    QAction* labelAboveAction = labelPosMenu->addAction("↑ Acima");
    QAction* labelBelowAction = labelPosMenu->addAction("↓ Abaixo");
    QAction* labelHiddenAction = labelPosMenu->addAction("⊗ Oculto");
    
    // Marcar posição atual
    labelRightAction->setCheckable(true);
    labelLeftAction->setCheckable(true);
    labelAboveAction->setCheckable(true);
    labelBelowAction->setCheckable(true);
    labelHiddenAction->setCheckable(true);
    
    switch (minutia->labelPosition) {
        case MinutiaLabelPosition::RIGHT: labelRightAction->setChecked(true); break;
        case MinutiaLabelPosition::LEFT: labelLeftAction->setChecked(true); break;
        case MinutiaLabelPosition::ABOVE: labelAboveAction->setChecked(true); break;
        case MinutiaLabelPosition::BELOW: labelBelowAction->setChecked(true); break;
        case MinutiaLabelPosition::HIDDEN: labelHiddenAction->setChecked(true); break;
    }
    
    menu.addSeparator();
    
    QAction* editAction = menu.addAction("✏️ Editar Propriedades...");
    QAction* deleteAction = menu.addAction("🗑 Excluir Minúcia");
    
    // Marcar ação atual
    if (editState == MinutiaEditState::EDITING_POSITION) {
        moveAction->setCheckable(true);
        moveAction->setChecked(true);
    } else if (editState == MinutiaEditState::EDITING_ANGLE) {
        rotateAction->setCheckable(true);
        rotateAction->setChecked(true);
    }
    
    // Executar menu e processar ação
    QAction* selectedAction = menu.exec(event->globalPos());
    
    if (selectedAction == moveAction) {
        editState = MinutiaEditState::EDITING_POSITION;
        emit editStateChanged(editState);
        update();
    } else if (selectedAction == rotateAction) {
        editState = MinutiaEditState::EDITING_ANGLE;
        initialAngle = minutia->angle;
        emit editStateChanged(editState);
        update();
    } else if (selectedAction == editAction) {
        emit minutiaDoubleClicked(minutia->id);
    } else if (selectedAction == deleteAction) {
        // Emitir sinal para deletar (será capturado pelo MainWindow)
        clearSelection();
    } else if (selectedAction == labelRightAction) {
        minutia->labelPosition = MinutiaLabelPosition::RIGHT;
        update();
    } else if (selectedAction == labelLeftAction) {
        minutia->labelPosition = MinutiaLabelPosition::LEFT;
        update();
    } else if (selectedAction == labelAboveAction) {
        minutia->labelPosition = MinutiaLabelPosition::ABOVE;
        update();
    } else if (selectedAction == labelBelowAction) {
        minutia->labelPosition = MinutiaLabelPosition::BELOW;
        update();
    } else if (selectedAction == labelHiddenAction) {
        minutia->labelPosition = MinutiaLabelPosition::HIDDEN;
        update();
    }
}

Minutia* MinutiaeOverlay::findMinutiaAt(const QPoint& pos) {
    if (!currentFragment) return nullptr;

    // Aumentar tolerância de clique para facilitar seleção
    // Usar o maior valor entre markerSize e uma margem mínima generosa
    int baseRadius = displaySettings.markerSize / 2;
    int clickRadius = qMax(baseRadius + 15, 30); // Mínimo de 30 pixels de tolerância
    
    qDebug() << "findMinutiaAt: pos=" << pos << "clickRadius=" << clickRadius << "minutiae count=" << currentFragment->minutiae.size();

    for (auto& minutia : currentFragment->minutiae) {
        QPoint scaledPos = scalePoint(minutia.position);
        int dx = pos.x() - scaledPos.x();
        int dy = pos.y() - scaledPos.y();
        int distance = static_cast<int>(std::sqrt(dx * dx + dy * dy));
        
        qDebug() << "  Minutia at" << scaledPos << "distance=" << distance << "id=" << minutia.id.left(8);

        if (distance <= clickRadius) {
            qDebug() << "  -> FOUND! Returning minutia" << minutia.id.left(8);
            return &minutia;
        }
    }

    qDebug() << "  -> No minutia found at click position";
    return nullptr;
}

QPoint MinutiaeOverlay::scalePoint(const QPoint& imagePoint) const {
    // 1. Escalar coordenadas da imagem
    // 2. Adicionar offset de centralização
    // 3. Subtrair offset de scroll
    return QPoint(
        static_cast<int>(imagePoint.x() * scaleFactor) + imageOffset.x() - scrollOffset.x(),
        static_cast<int>(imagePoint.y() * scaleFactor) + imageOffset.y() - scrollOffset.y()
    );
}

QPoint MinutiaeOverlay::unscalePoint(const QPoint& widgetPoint) const {
    // 1. Adicionar offset de scroll
    // 2. Subtrair offset de centralização
    // 3. Descalar para coordenadas da imagem
    return QPoint(
        static_cast<int>((widgetPoint.x() + scrollOffset.x() - imageOffset.x()) / scaleFactor),
        static_cast<int>((widgetPoint.y() + scrollOffset.y() - imageOffset.y()) / scaleFactor)
    );
}

void MinutiaeOverlay::drawMinutiaWithArrow(QPainter& painter, const QPoint& pos, float angle, const QColor& color, bool isSelected) {
    int size = displaySettings.markerSize;
    int penWidth = isSelected ? 3 : 2;
    
    QPen pen(color, penWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    
    // Desenhar círculo
    int radius = size / 2;
    painter.drawEllipse(pos, radius, radius);
    
    // Desenhar seta indicando o ângulo
    double radians = angle * M_PI / 180.0;
    int arrowLength = static_cast<int>(radius * 1.5);
    
    int endX = pos.x() + static_cast<int>(arrowLength * cos(radians));
    int endY = pos.y() - static_cast<int>(arrowLength * sin(radians));
    
    // Linha da seta
    painter.drawLine(pos, QPoint(endX, endY));
    
    // Pontas da seta
    double arrowHeadSize = size / 4.0;
    double arrowAngle1 = radians + M_PI * 3.0 / 4.0;
    double arrowAngle2 = radians - M_PI * 3.0 / 4.0;
    
    QPoint arrowHead1(endX + static_cast<int>(arrowHeadSize * cos(arrowAngle1)),
                     endY - static_cast<int>(arrowHeadSize * sin(arrowAngle1)));
    QPoint arrowHead2(endX + static_cast<int>(arrowHeadSize * cos(arrowAngle2)),
                     endY - static_cast<int>(arrowHeadSize * sin(arrowAngle2)));
    
    painter.drawLine(endX, endY, arrowHead1.x(), arrowHead1.y());
    painter.drawLine(endX, endY, arrowHead2.x(), arrowHead2.y());
    
    // Se estiver selecionada, desenhar círculo adicional de destaque
    if (isSelected) {
        QPen highlightPen(color, 1, Qt::DashLine);
        painter.setPen(highlightPen);
        painter.drawEllipse(pos, radius + 5, radius + 5);
    }
}

void MinutiaeOverlay::drawEditStateIndicator(QPainter& painter, const QPoint& pos) {
    QString stateText;
    QColor indicatorColor = selectedColor;
    
    switch (editState) {
        case MinutiaEditState::SELECTED:
            stateText = "🔘 SELECIONADA - Botão direito para opções";
            break;
        case MinutiaEditState::EDITING_POSITION:
            stateText = "↔️ MOVER - Arraste com botão esquerdo | Clique p/ alternar";
            indicatorColor = QColor(0, 200, 255); // Azul claro
            break;
        case MinutiaEditState::EDITING_ANGLE:
            stateText = "🔄 ROTACIONAR - Arraste com botão esquerdo | Clique p/ alternar";
            indicatorColor = QColor(255, 165, 0); // Laranja
            break;
        default:
            return;
    }
    
    // Desenhar texto de instrução
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(10);
    painter.setFont(font);
    
    QRect textRect = painter.fontMetrics().boundingRect(stateText);
    QPoint textPos(pos.x() - textRect.width() / 2, pos.y() - displaySettings.markerSize - 25);
    
    // Fundo semi-transparente
    QColor bgColor = indicatorColor;
    bgColor.setAlpha(180);
    painter.fillRect(textRect.translated(textPos).adjusted(-5, -2, 5, 2), bgColor);
    
    // Texto
    painter.setPen(Qt::black);
    painter.drawText(textPos, stateText);
    
    // Linha conectando ao marcador
    QPen dashedPen(indicatorColor, 2, Qt::DashLine);
    painter.setPen(dashedPen);
    painter.drawLine(pos.x(), pos.y() - displaySettings.markerSize/2 - 3, 
                    pos.x(), pos.y() - displaySettings.markerSize - 20);
}

float MinutiaeOverlay::calculateAngleFromDrag(const QPoint& center, const QPoint& dragPos) const {
    // Calcular ângulo entre o centro da minúcia e a posição do mouse
    int dx = dragPos.x() - center.x();
    int dy = center.y() - dragPos.y();  // Invertido porque Y cresce para baixo
    
    // atan2 retorna radianos entre -π e π
    double radians = std::atan2(dy, dx);
    
    // Converter para graus (0-360)
    float degrees = static_cast<float>(radians * 180.0 / M_PI);
    
    // Normalizar para 0-360
    if (degrees < 0) {
        degrees += 360.0f;
    }
    
    return degrees;
}

} // namespace FingerprintEnhancer
