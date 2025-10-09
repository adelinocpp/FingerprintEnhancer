#include "MinutiaeOverlay.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>

namespace FingerprintEnhancer {

MinutiaeOverlay::MinutiaeOverlay(QWidget *parent)
    : QWidget(parent),
      currentFragment(nullptr),
      scaleFactor(1.0),
      scrollOffset(0, 0),
      imageOffset(0, 0),
      editMode(false),
      draggingMinutia(false),
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
    draggingMinutia = false;
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
        drawMinutia(painter, minutia, isSelected);

        if (showLabels) {
            QPoint scaledPos = scalePoint(minutia.position);
            drawMinutiaLabel(painter, scaledPos, number, minutia.getTypeAbbreviation(), isSelected);
        }

        number++;
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


void MinutiaeOverlay::drawMinutiaLabel(QPainter& painter, const QPoint& pos, int number, const QString& type, bool isSelected) {
    QColor textColor = isSelected ? selectedColor : normalColor;
    QFont font = painter.font();
    font.setPointSize(displaySettings.labelFontSize);
    font.setBold(isSelected);
    painter.setFont(font);

    int size = displaySettings.markerSize;

    // Desenhar número com espaços
    QString label = QString(" %1 .").arg(number);
    QRect textRect = painter.fontMetrics().boundingRect(label);
    QPoint textPos(pos.x() + size/2 + 5, pos.y() - size/2);

    // Fundo configurável para melhor legibilidade
    painter.fillRect(textRect.translated(textPos), displaySettings.labelBackgroundColor);
    painter.setPen(textColor);
    painter.drawText(textPos, label);

    // Desenhar tipo (abreviação) com espaços
    if (!type.isEmpty() && type != "N/C") {
        QString typeLabel = QString(" %1 .").arg(type);
        QPoint typePos(pos.x() + size/2 + 5, pos.y() + 5);
        QRect typeRect = painter.fontMetrics().boundingRect(typeLabel);
        painter.fillRect(typeRect.translated(typePos), displaySettings.labelBackgroundColor);
        painter.drawText(typePos, typeLabel);
    }
}

void MinutiaeOverlay::mousePressEvent(QMouseEvent *event) {
    if (!currentFragment || !editMode) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        Minutia* minutia = findMinutiaAt(event->pos());
        if (minutia) {
            selectedMinutiaId = minutia->id;
            draggingMinutia = true;
            dragStartPos = event->pos();
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
    if (draggingMinutia && !selectedMinutiaId.isEmpty() && editMode) {
        QPoint newPos = unscalePoint(event->pos());
        emit positionChanged(selectedMinutiaId, newPos);
        update();
    }

    QWidget::mouseMoveEvent(event);
}

void MinutiaeOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        draggingMinutia = false;
    }

    QWidget::mouseReleaseEvent(event);
}

Minutia* MinutiaeOverlay::findMinutiaAt(const QPoint& pos) {
    if (!currentFragment) return nullptr;

    int clickRadius = displaySettings.markerSize / 2 + 5; // Margem de tolerância

    for (auto& minutia : currentFragment->minutiae) {
        QPoint scaledPos = scalePoint(minutia.position);
        int dx = pos.x() - scaledPos.x();
        int dy = pos.y() - scaledPos.y();
        int distance = static_cast<int>(std::sqrt(dx * dx + dy * dy));

        if (distance <= clickRadius) {
            return &minutia;
        }
    }

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

} // namespace FingerprintEnhancer
