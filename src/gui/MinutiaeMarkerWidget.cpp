#include "MinutiaeMarkerWidget.h"
#include <QPainter>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <cmath>

MinutiaeMarkerWidget::MinutiaeMarkerWidget(QWidget *parent)
    : QWidget(parent),
      currentMode(Mode::View),
      currentType(MinutiaeType::BIFURCATION),
      nextId(0),
      showNumbers(true),
      showAngles(true),
      showLabels(false),
      zoomFactor(1.0),
      selectedMinutiaId(-1),
      draggingMinutia(false),
      minutiaRadius(12),
      lineLength(20)
{
    setMouseTracking(true);
    labelFont = QFont("Arial", 12);
    numberFont = QFont("Arial", 14, QFont::Bold);
}

MinutiaeMarkerWidget::~MinutiaeMarkerWidget() {
}

void MinutiaeMarkerWidget::setImage(const cv::Mat &image) {
    if (image.empty()) {
        baseImage = cv::Mat();
        setMinimumSize(100, 100);
    } else {
        baseImage = image.clone();
        setMinimumSize(image.cols * zoomFactor, image.rows * zoomFactor);
    }
    update();
}

void MinutiaeMarkerWidget::addMinutia(const MinutiaeData &minutia) {
    MinutiaeData newMinutia = minutia;
    newMinutia.id = nextId++;
    minutiae.append(newMinutia);
    emit minutiaAdded(newMinutia);
    emit minutiaeChanged();
    update();
}

void MinutiaeMarkerWidget::removeMinutia(int id) {
    for (int i = 0; i < minutiae.size(); i++) {
        if (minutiae[i].id == id) {
            minutiae.removeAt(i);
            emit minutiaRemoved(id);
            emit minutiaeChanged();
            update();
            break;
        }
    }
}

void MinutiaeMarkerWidget::updateMinutia(int id, const MinutiaeData &minutia) {
    for (int i = 0; i < minutiae.size(); i++) {
        if (minutiae[i].id == id) {
            minutiae[i] = minutia;
            minutiae[i].id = id;  // Preservar ID
            emit minutiaUpdated(id, minutia);
            emit minutiaeChanged();
            update();
            break;
        }
    }
}

void MinutiaeMarkerWidget::clearMinutiae() {
    minutiae.clear();
    nextId = 0;
    emit minutiaeChanged();
    update();
}

void MinutiaeMarkerWidget::setMinutiae(const QVector<MinutiaeData> &newMinutiae) {
    minutiae = newMinutiae;

    // Atualizar nextId
    nextId = 0;
    for (const auto &m : minutiae) {
        if (m.id >= nextId) {
            nextId = m.id + 1;
        }
    }

    emit minutiaeChanged();
    update();
}

void MinutiaeMarkerWidget::setMode(Mode mode) {
    currentMode = mode;
    emit modeChanged(mode);

    // Atualizar cursor
    if (mode == Mode::AddMinutia) {
        setCursor(Qt::CrossCursor);
    } else if (mode == Mode::RemoveMinutia) {
        setCursor(Qt::ForbiddenCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void MinutiaeMarkerWidget::setCurrentMinutiaeType(MinutiaeType type) {
    currentType = type;
}

void MinutiaeMarkerWidget::setShowNumbers(bool show) {
    showNumbers = show;
    update();
}

void MinutiaeMarkerWidget::setShowAngles(bool show) {
    showAngles = show;
    update();
}

void MinutiaeMarkerWidget::setShowLabels(bool show) {
    showLabels = show;
    update();
}

void MinutiaeMarkerWidget::setZoomFactor(double factor) {
    zoomFactor = std::max(0.1, std::min(10.0, factor));
    if (!baseImage.empty()) {
        setMinimumSize(baseImage.cols * zoomFactor, baseImage.rows * zoomFactor);
    }
    update();
}

cv::Mat MinutiaeMarkerWidget::renderMinutiaeImage(bool includeNumbers,
                                                   bool includeAngles,
                                                   bool includeLabels) {
    if (baseImage.empty()) {
        return cv::Mat();
    }

    cv::Mat result;
    if (baseImage.channels() == 1) {
        cv::cvtColor(baseImage, result, cv::COLOR_GRAY2BGR);
    } else {
        result = baseImage.clone();
    }

    for (int i = 0; i < minutiae.size(); i++) {
        const auto &m = minutiae[i];
        cv::Point center(m.position.x, m.position.y);

        // Cor baseada no tipo
        cv::Scalar color = MinutiaeTypeInfo::getTypeColor(m.type);

        // Desenhar círculo
        cv::circle(result, center, minutiaRadius, color, 2);

        // Desenhar ângulo
        if (includeAngles && m.angle != 0.0f) {
            int len = lineLength;
            cv::Point endPoint(
                center.x + len * std::cos(m.angle),
                center.y + len * std::sin(m.angle)
            );
            cv::line(result, center, endPoint, color, 2);
        }

        // Desenhar número
        if (includeNumbers) {
            QString numText = QString::number(i + 1);
            cv::Point textPos(center.x + 16, center.y - 1);
            cv::putText(result, numText.toStdString(), textPos,
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);

            // Desenhar linha conectando ao número
            cv::line(result, center, cv::Point(center.x + 8, center.y - 8),
                    color, 1);
        }

        // Desenhar label do tipo
        if (includeLabels) {
            QString typeText = MinutiaeTypeInfo::getTypeName(m.type);
            cv::Point labelPos(center.x + 16, center.y + 36);
            cv::putText(result, typeText.toStdString(), labelPos,
                       cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
        }
    }

    return result;
}

void MinutiaeMarkerWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::gray);

    if (baseImage.empty()) {
        painter.drawText(rect(), Qt::AlignCenter, "Nenhuma imagem carregada");
        return;
    }

    // Desenhar imagem base
    QImage qimg;
    if (baseImage.channels() == 1) {
        qimg = QImage(baseImage.data, baseImage.cols, baseImage.rows,
                     baseImage.step, QImage::Format_Grayscale8).copy();
    } else {
        cv::Mat rgb;
        cv::cvtColor(baseImage, rgb, cv::COLOR_BGR2RGB);
        qimg = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step,
                     QImage::Format_RGB888).copy();
    }

    QRect imageRect(0, 0, qimg.width() * zoomFactor, qimg.height() * zoomFactor);
    painter.drawImage(imageRect, qimg);

    // Desenhar minúcias
    drawMinutiae(painter);
}

void MinutiaeMarkerWidget::drawMinutiae(QPainter &painter) {
    for (int i = 0; i < minutiae.size(); i++) {
        drawMinutia(painter, minutiae[i], i + 1);
    }
}

void MinutiaeMarkerWidget::drawMinutia(QPainter &painter,
                                       const MinutiaeData &minutia,
                                       int index) {
    QPoint center = imageToWidget(minutia.position);
    QColor color = getMinutiaColor(minutia.type);

    // Destacar se selecionada
    bool isSelected = (minutia.id == selectedMinutiaId);
    int radius = isSelected ? minutiaRadius + 2 : minutiaRadius;

    // Desenhar círculo
    painter.setPen(QPen(color, 2));
    if (isSelected) {
        painter.setBrush(QBrush(color, Qt::DiagCrossPattern));
    } else {
        painter.setBrush(Qt::NoBrush);
    }
    painter.drawEllipse(center, radius, radius);

    // Desenhar ângulo
    if (showAngles && minutia.angle != 0.0f) {
        int len = lineLength;
        QPoint endPoint(
            center.x() + len * std::cos(minutia.angle),
            center.y() + len * std::sin(minutia.angle)
        );
        painter.drawLine(center, endPoint);
    }

    // Desenhar número
    if (showNumbers) {
        drawMinutiaNumber(painter, minutia, index);
    }

    // Desenhar label
    if (showLabels) {
        drawMinutiaLabel(painter, minutia);
    }
}

void MinutiaeMarkerWidget::drawMinutiaNumber(QPainter &painter,
                                             const MinutiaeData &minutia,
                                             int number) {
    QPoint center = imageToWidget(minutia.position);
    QColor color = getMinutiaColor(minutia.type);

    // Posição do número (acima e à direita)
    QPoint numPos(center.x() + 10, center.y() - 10);

    // Desenhar linha conectando
    painter.setPen(QPen(color, 1, Qt::DashLine));
    painter.drawLine(center, QPoint(center.x() + 8, center.y() - 8));

    // Desenhar número
    painter.setFont(numberFont);
    painter.setPen(color);
    painter.drawText(numPos, QString::number(number));
}

void MinutiaeMarkerWidget::drawMinutiaAngle(QPainter &painter,
                                            const MinutiaeData &minutia) {
    // Já desenhado em drawMinutia
}

void MinutiaeMarkerWidget::drawMinutiaLabel(QPainter &painter,
                                            const MinutiaeData &minutia) {
    QPoint center = imageToWidget(minutia.position);
    QColor color = getMinutiaColor(minutia.type);

    QString label = MinutiaeTypeInfo::getTypeName(minutia.type);
    QPoint labelPos(center.x() + 10, center.y() + 20);

    painter.setFont(labelFont);
    painter.setPen(color);
    painter.drawText(labelPos, label);
}

void MinutiaeMarkerWidget::mousePressEvent(QMouseEvent *event) {
    if (baseImage.empty()) {
        return;
    }

    cv::Point2f imagePos = widgetToImage(event->pos());

    if (event->button() == Qt::LeftButton) {
        if (currentMode == Mode::AddMinutia) {
            // Adicionar nova minúcia
            MinutiaeData minutia;
            minutia.position = imagePos;
            minutia.angle = 0.0f;  // Pode ser ajustado depois
            minutia.type = currentType;
            minutia.quality = 1.0f;
            addMinutia(minutia);

        } else if (currentMode == Mode::RemoveMinutia) {
            // Remover minúcia
            int id = findMinutiaAt(event->pos());
            if (id >= 0) {
                removeMinutia(id);
            }

        } else if (currentMode == Mode::EditMinutia) {
            // Selecionar para editar
            selectedMinutiaId = findMinutiaAt(event->pos());
            if (selectedMinutiaId >= 0) {
                draggingMinutia = true;
                lastMousePos = event->pos();
            }
            update();
        }
    }
}

void MinutiaeMarkerWidget::mouseMoveEvent(QMouseEvent *event) {
    if (draggingMinutia && selectedMinutiaId >= 0) {
        // Mover minúcia selecionada
        cv::Point2f newPos = widgetToImage(event->pos());

        for (auto &m : minutiae) {
            if (m.id == selectedMinutiaId) {
                m.position = newPos;
                emit minutiaUpdated(m.id, m);
                update();
                break;
            }
        }
    }
}

void MinutiaeMarkerWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        draggingMinutia = false;
    }
}

void MinutiaeMarkerWidget::contextMenuEvent(QContextMenuEvent *event) {
    int id = findMinutiaAt(event->pos());

    if (id < 0) {
        return;  // Nenhuma minúcia encontrada
    }

    QMenu menu(this);

    // Encontrar minúcia
    MinutiaeData *minutia = nullptr;
    for (auto &m : minutiae) {
        if (m.id == id) {
            minutia = &m;
            break;
        }
    }

    if (!minutia) return;

    // Opções do menu
    QAction *changeTypeAction = menu.addAction("Alterar Tipo");
    QAction *changeAngleAction = menu.addAction("Ajustar Ângulo");
    menu.addSeparator();
    QAction *deleteAction = menu.addAction("Remover");

    QAction *selected = menu.exec(event->globalPos());

    if (selected == changeTypeAction) {
        // Diálogo para selecionar tipo
        QStringList types = MinutiaeTypeInfo::getAllTypeNames();
        bool ok;
        QString selectedType = QInputDialog::getItem(
            this, "Tipo de Minúcia",
            "Selecione o tipo:", types, 0, false, &ok);

        if (ok) {
            minutia->type = MinutiaeTypeInfo::getTypeFromName(selectedType);
            emit minutiaUpdated(minutia->id, *minutia);
            update();
        }

    } else if (selected == changeAngleAction) {
        // Diálogo para ajustar ângulo
        bool ok;
        double angleDeg = QInputDialog::getDouble(
            this, "Ângulo da Minúcia",
            "Ângulo (graus):", minutia->angle * 180.0 / M_PI,
            0, 360, 1, &ok);

        if (ok) {
            minutia->angle = angleDeg * M_PI / 180.0;
            emit minutiaUpdated(minutia->id, *minutia);
            update();
        }

    } else if (selected == deleteAction) {
        removeMinutia(id);
    }
}

int MinutiaeMarkerWidget::findMinutiaAt(const QPoint &pos, int tolerance) {
    for (const auto &m : minutiae) {
        QPoint center = imageToWidget(m.position);
        int dx = center.x() - pos.x();
        int dy = center.y() - pos.y();
        int distance = std::sqrt(dx*dx + dy*dy);

        if (distance <= tolerance) {
            return m.id;
        }
    }
    return -1;
}

QPoint MinutiaeMarkerWidget::imageToWidget(const cv::Point2f &imagePoint) const {
    return QPoint(imagePoint.x * zoomFactor, imagePoint.y * zoomFactor);
}

cv::Point2f MinutiaeMarkerWidget::widgetToImage(const QPoint &widgetPoint) const {
    return cv::Point2f(widgetPoint.x() / zoomFactor,
                       widgetPoint.y() / zoomFactor);
}

void MinutiaeMarkerWidget::showContextMenu(const QPoint &pos) {
    // Implementado em contextMenuEvent
}

QColor MinutiaeMarkerWidget::getMinutiaColor(MinutiaeType type) const {
    cv::Scalar cvColor = MinutiaeTypeInfo::getTypeColor(type);
    // OpenCV usa BGR, Qt usa RGB
    return QColor(cvColor[2], cvColor[1], cvColor[0]);
}
