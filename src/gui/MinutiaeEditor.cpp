#include "MinutiaeEditor.h"
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtCore/QDebug>
#include <algorithm>
#include <cmath>

MinutiaeEditor::MinutiaeEditor(QWidget *parent) 
    : ImageViewer(parent), 
      currentMode(VIEW_MODE),
      showMinutiae(true),
      showAngles(true), 
      showIds(true),
      showComparison(false),
      minutiaSize(5),
      selectedMinutiaId(-1),
      draggingMinutia(false) {
    setupDrawingStyles();
}

MinutiaeEditor::~MinutiaeEditor() = default;

void MinutiaeEditor::setEditMode(EditMode mode) {
    currentMode = mode;
    update();
}

MinutiaeEditor::EditMode MinutiaeEditor::getEditMode() const {
    return currentMode;
}

void MinutiaeEditor::setMinutiae(const std::vector<Minutia> &minutiae) {
    currentMinutiae = minutiae;
    update();
}

std::vector<Minutia> MinutiaeEditor::getMinutiae() const {
    return currentMinutiae;
}

void MinutiaeEditor::clearMinutiae() {
    currentMinutiae.clear();
    selectedMinutiaId = -1;
    update();
}

void MinutiaeEditor::setShowMinutiae(bool show) {
    showMinutiae = show;
    update();
}

void MinutiaeEditor::setShowAngles(bool show) {
    showAngles = show;
    update();
}

void MinutiaeEditor::setShowIds(bool show) {
    showIds = show;
    update();
}

void MinutiaeEditor::setMinutiaSize(int size) {
    minutiaSize = size;
    update();
}

void MinutiaeEditor::setComparisonMinutiae(const std::vector<Minutia> &minutiae) {
    comparisonMinutiae = minutiae;
    update();
}

void MinutiaeEditor::setShowComparison(bool show) {
    showComparison = show;
    update();
}

void MinutiaeEditor::highlightMatchedMinutiae(const std::vector<std::pair<int, int>> &matches) {
    matchedPairs = matches;
    update();
}

void MinutiaeEditor::paintEvent(QPaintEvent *event) {
    ImageViewer::paintEvent(event);
    
    if (showMinutiae && hasImage()) {
        QPainter painter(viewport());
        drawMinutiae(painter);
    }
}

void MinutiaeEditor::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPoint imagePos = widgetToImage(event->pos());
        
        switch (currentMode) {
            case ADD_TERMINATION:
                {
                    Minutia newMinutia = createMinutiaAt(imagePos, 0); // 0 = terminação
                    currentMinutiae.push_back(newMinutia);
                    emit minutiaAdded(newMinutia);
                    update();
                }
                break;
                
            case ADD_BIFURCATION:
                {
                    Minutia newMinutia = createMinutiaAt(imagePos, 1); // 1 = bifurcação
                    currentMinutiae.push_back(newMinutia);
                    emit minutiaAdded(newMinutia);
                    update();
                }
                break;
                
            case EDIT_MODE:
                {
                    int minutiaId = findMinutiaAt(imagePos);
                    if (minutiaId >= 0) {
                        selectMinutia(minutiaId);
                        draggingMinutia = true;
                        dragStartPoint = imagePos;
                    }
                }
                break;
                
            case DELETE_MODE:
                {
                    int minutiaId = findMinutiaAt(imagePos);
                    if (minutiaId >= 0) {
                        deleteMinutia(minutiaId);
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    ImageViewer::mousePressEvent(event);
}

void MinutiaeEditor::mouseMoveEvent(QMouseEvent *event) {
    if (draggingMinutia && selectedMinutiaId >= 0) {
        QPoint imagePos = widgetToImage(event->pos());
        moveMinutia(selectedMinutiaId, imagePos);
        update();
    }
    
    ImageViewer::mouseMoveEvent(event);
}

void MinutiaeEditor::mouseReleaseEvent(QMouseEvent *event) {
    if (draggingMinutia) {
        draggingMinutia = false;
        // Emit signal about minutia modification
        for (const auto& minutia : currentMinutiae) {
            if (minutia.id == selectedMinutiaId) {
                emit minutiaModified(selectedMinutiaId, minutia);
                break;
            }
        }
    }
    
    ImageViewer::mouseReleaseEvent(event);
}

void MinutiaeEditor::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Delete:
            if (selectedMinutiaId >= 0) {
                deleteMinutia(selectedMinutiaId);
            }
            break;
            
        case Qt::Key_Escape:
            selectedMinutiaId = -1;
            draggingMinutia = false;
            update();
            break;
            
        default:
            ImageViewer::keyPressEvent(event);
            break;
    }
}

void MinutiaeEditor::onImageClicked(QPoint imagePosition) {
    // Método slot para responder a cliques na imagem
    emit minutiaSelected(-1); // Deselecionar por padrão
}

void MinutiaeEditor::drawMinutiae(QPainter &painter) {
    // Desenhar minúcias principais
    for (const auto& minutia : currentMinutiae) {
        drawMinutia(painter, minutia, false);
    }
    
    // Desenhar minúcias de comparação se habilitado
    if (showComparison) {
        for (const auto& minutia : comparisonMinutiae) {
            drawMinutia(painter, minutia, true);
        }
        
        // Desenhar linhas de correspondência
        drawMatchLines(painter);
    }
}

void MinutiaeEditor::drawMinutia(QPainter &painter, const Minutia &minutia, bool isComparison) {
    QPoint center = imageToWidget(QPoint(static_cast<int>(minutia.position.x), 
                                        static_cast<int>(minutia.position.y)));
    
    // Escolher cor e estilo baseado no tipo e estado
    QPen pen;
    if (isComparison) {
        pen = comparisonPen;
    } else if (minutia.id == selectedMinutiaId) {
        pen = selectedPen;
    } else if (minutia.type == 0) {
        pen = terminationPen;
    } else {
        pen = bifurcationPen;
    }
    
    painter.setPen(pen);
    
    // Desenhar círculo
    painter.drawEllipse(center, minutiaSize, minutiaSize);
    
    // Desenhar ângulo se habilitado
    if (showAngles) {
        drawMinutiaAngle(painter, minutia);
    }
    
    // Desenhar ID se habilitado
    if (showIds && !isComparison) {
        drawMinutiaId(painter, minutia);
    }
}

void MinutiaeEditor::drawMinutiaAngle(QPainter &painter, const Minutia &minutia) {
    QPoint center = imageToWidget(QPoint(static_cast<int>(minutia.position.x), 
                                        static_cast<int>(minutia.position.y)));
    
    int lineLength = minutiaSize * 3;
    QPoint endPoint(
        center.x() + static_cast<int>(lineLength * cos(minutia.angle)),
        center.y() + static_cast<int>(lineLength * sin(minutia.angle))
    );
    
    painter.setPen(anglePen);
    painter.drawLine(center, endPoint);
}

void MinutiaeEditor::drawMinutiaId(QPainter &painter, const Minutia &minutia) {
    QPoint center = imageToWidget(QPoint(static_cast<int>(minutia.position.x), 
                                        static_cast<int>(minutia.position.y)));
    
    painter.setPen(QPen(Qt::black));
    painter.drawText(center + QPoint(minutiaSize + 2, -minutiaSize - 2), 
                    QString::number(minutia.id));
}

void MinutiaeEditor::drawMatchLines(QPainter &painter) {
    painter.setPen(matchedPen);
    
    for (const auto& match : matchedPairs) {
        // Encontrar minúcias correspondentes
        const Minutia* m1 = nullptr;
        const Minutia* m2 = nullptr;
        
        for (const auto& minutia : currentMinutiae) {
            if (minutia.id == match.first) {
                m1 = &minutia;
                break;
            }
        }
        
        for (const auto& minutia : comparisonMinutiae) {
            if (minutia.id == match.second) {
                m2 = &minutia;
                break;
            }
        }
        
        if (m1 && m2) {
            QPoint p1 = imageToWidget(QPoint(static_cast<int>(m1->position.x), 
                                           static_cast<int>(m1->position.y)));
            QPoint p2 = imageToWidget(QPoint(static_cast<int>(m2->position.x), 
                                           static_cast<int>(m2->position.y)));
            painter.drawLine(p1, p2);
        }
    }
}

int MinutiaeEditor::findMinutiaAt(const QPoint &position, int tolerance) {
    for (const auto& minutia : currentMinutiae) {
        QPoint minutiaPos(static_cast<int>(minutia.position.x), 
                         static_cast<int>(minutia.position.y));
        
        if ((position - minutiaPos).manhattanLength() <= tolerance) {
            return minutia.id;
        }
    }
    return -1;
}

Minutia MinutiaeEditor::createMinutiaAt(const QPoint &position, int type) {
    static int nextId = 0;
    return Minutia(cv::Point2f(position.x(), position.y()), 0.0f, type, 1.0f, nextId++);
}

void MinutiaeEditor::selectMinutia(int id) {
    selectedMinutiaId = id;
    emit minutiaSelected(id);
    update();
}

void MinutiaeEditor::moveMinutia(int id, const QPoint &newPosition) {
    for (auto& minutia : currentMinutiae) {
        if (minutia.id == id) {
            minutia.position = cv::Point2f(newPosition.x(), newPosition.y());
            break;
        }
    }
}

void MinutiaeEditor::deleteMinutia(int id) {
    currentMinutiae.erase(
        std::remove_if(currentMinutiae.begin(), currentMinutiae.end(),
            [id](const Minutia& m) { return m.id == id; }),
        currentMinutiae.end());
    
    if (selectedMinutiaId == id) {
        selectedMinutiaId = -1;
    }
    
    emit minutiaDeleted(id);
    update();
}

QPoint MinutiaeEditor::minutiaToWidget(const Minutia &minutia) const {
    return imageToWidget(QPoint(static_cast<int>(minutia.position.x), 
                               static_cast<int>(minutia.position.y)));
}

QPoint MinutiaeEditor::widgetToMinutia(const QPoint &widgetPoint) const {
    return widgetToImage(widgetPoint);
}

void MinutiaeEditor::updateMinutiaAngles() {
    // TODO: Implementar cálculo automático de ângulos
}

int MinutiaeEditor::getNextMinutiaId() const {
    int maxId = -1;
    for (const auto& minutia : currentMinutiae) {
        maxId = std::max(maxId, minutia.id);
    }
    return maxId + 1;
}

void MinutiaeEditor::setupDrawingStyles() {
    terminationPen = QPen(Qt::red, 2);
    bifurcationPen = QPen(Qt::blue, 2);
    selectedPen = QPen(Qt::yellow, 3);
    comparisonPen = QPen(Qt::green, 2);
    matchedPen = QPen(Qt::magenta, 1, Qt::DashLine);
    anglePen = QPen(Qt::darkGray, 1);
}

