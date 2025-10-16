#include "CropTool.h"
#include <QCursor>
#include <QKeyEvent>
#include <QPolygon>
#include <algorithm>

CropTool::CropTool(QObject *parent)
    : QObject(parent),
      state(State::Inactive),
      selectionBorderColor(Qt::yellow),
      selectionFillColor(QColor(255, 255, 0, 30)),
      handleColor(Qt::yellow),
      handleSize(8),
      borderWidth(2)
{
}

CropTool::~CropTool() {
}

void CropTool::setActive(bool active) {
    if (active) {
        if (state == State::Inactive) {
            state = State::Selecting;
        }
    } else {
        state = State::Inactive;
        clearSelection();
    }
}

void CropTool::setSelectionRect(const QRect &rect) {
    selectionRect = rect;
    if (!rect.isEmpty()) {
        state = State::Selected;
        emit selectionChanged(selectionRect);
    }
}

void CropTool::clearSelection() {
    selectionRect = QRect();
    startPoint = QPoint();
    currentPoint = QPoint();
    if (state != State::Inactive) {
        state = State::Selecting;
    }
    emit selectionChanged(QRect());
}

void CropTool::draw(QPainter &painter, double scaleFactor) {
    if (state == State::Inactive || selectionRect.isEmpty()) {
        return;
    }

    painter.save();

    // Desenhar retângulo de seleção
    QPen borderPen(selectionBorderColor, borderWidth);
    borderPen.setCosmetic(true);  // Não afetado por transformações
    painter.setPen(borderPen);
    painter.setBrush(QBrush(selectionFillColor));
    painter.drawRect(selectionRect);

    // Desenhar handles de redimensionamento se selecionado
    if (state == State::Selected || state == State::Moving || state == State::MovingWithMouseOnly) {
        painter.setBrush(QBrush(handleColor));
        painter.setPen(QPen(Qt::black, 1));

        // Cantos (quadrados)
        painter.drawRect(getHandleRect(selectionRect.topLeft(), scaleFactor));
        painter.drawRect(getHandleRect(selectionRect.topRight(), scaleFactor));
        painter.drawRect(getHandleRect(selectionRect.bottomLeft(), scaleFactor));
        painter.drawRect(getHandleRect(selectionRect.bottomRight(), scaleFactor));

        // Lados (LOSANGOS/DIAMANTES) - Novidade!
        int diamondSize = handleSize + 4;  // Losangos um pouco maiores
        drawDiamondHandle(painter, QPoint(selectionRect.center().x(), selectionRect.top()), diamondSize);
        drawDiamondHandle(painter, QPoint(selectionRect.center().x(), selectionRect.bottom()), diamondSize);
        drawDiamondHandle(painter, QPoint(selectionRect.left(), selectionRect.center().y()), diamondSize);
        drawDiamondHandle(painter, QPoint(selectionRect.right(), selectionRect.center().y()), diamondSize);
    }

    painter.restore();
}

bool CropTool::handleMousePress(QMouseEvent *event, double scaleFactor) {
    if (state == State::Inactive) {
        return false;
    }

    QPoint pos = event->pos();

    // Verificar se clicou em um handle de redimensionamento
    if (state == State::Selected) {
        State resizeState = getResizeState(pos, scaleFactor);
        if (resizeState != State::Selected) {
            state = resizeState;
            lastMousePos = pos;
            return true;
        }

        // Verificar se clicou dentro da seleção para mover
        if (selectionRect.contains(pos)) {
            state = State::Moving;
            lastMousePos = pos;
            return true;
        }
    }

    // Iniciar nova seleção
    if (event->button() == Qt::LeftButton) {
        startPoint = pos;
        currentPoint = pos;
        selectionRect = QRect(startPoint, currentPoint).normalized();
        state = State::Selecting;
        return true;
    }

    return false;
}

bool CropTool::handleMouseMove(QMouseEvent *event, double scaleFactor) {
    if (state == State::Inactive) {
        return false;
    }

    QPoint pos = event->pos();
    QPoint delta = pos - lastMousePos;

    switch (state) {
        case State::Selecting:
            currentPoint = pos;
            selectionRect = QRect(startPoint, currentPoint).normalized();
            emit selectionChanged(selectionRect);
            return true;

        case State::Moving:
            moveSelection(delta);
            lastMousePos = pos;
            emit selectionChanged(selectionRect);
            return true;
        
        case State::MovingWithMouseOnly:
            // Modo especial: move apenas com movimento do mouse (sem segurar botão)
            moveSelection(delta);
            lastMousePos = pos;
            emit selectionChanged(selectionRect);
            return true;

        case State::ResizingTopLeft:
        case State::ResizingTopRight:
        case State::ResizingBottomLeft:
        case State::ResizingBottomRight:
        case State::ResizingTop:
        case State::ResizingBottom:
        case State::ResizingLeft:
        case State::ResizingRight:
            resizeSelection(delta, state);
            lastMousePos = pos;
            emit selectionChanged(selectionRect);
            return true;

        default:
            break;
    }

    return false;
}

bool CropTool::handleMouseRelease(QMouseEvent *event, double scaleFactor) {
    if (state == State::Inactive) {
        return false;
    }

    if (event->button() == Qt::LeftButton) {
        if (state == State::Selecting) {
            currentPoint = event->pos();
            selectionRect = QRect(startPoint, currentPoint).normalized();

            // Validar seleção (mínimo 10x10 pixels)
            if (selectionRect.width() >= 10 && selectionRect.height() >= 10) {
                state = State::Selected;
                emit selectionFinished(selectionRect);
            } else {
                clearSelection();
            }
        } else {
            state = State::Selected;
        }
        return true;
    }

    return false;
}

cv::Mat CropTool::cropImage(const cv::Mat &image) const {
    if (image.empty() || selectionRect.isEmpty()) {
        return cv::Mat();
    }

    // Garantir que a seleção está dentro dos limites da imagem
    QRect imageRect(0, 0, image.cols, image.rows);
    QRect validRect = selectionRect.intersected(imageRect);

    if (validRect.isEmpty()) {
        return cv::Mat();
    }

    // Criar ROI e clonar
    cv::Rect roi(validRect.x(), validRect.y(), validRect.width(), validRect.height());
    return image(roi).clone();
}

CropTool::State CropTool::getResizeState(const QPoint &pos, double scaleFactor) const {
    if (selectionRect.isEmpty()) {
        return State::Selected;
    }

    int tolerance = handleSize + 2;

    // Verificar cantos
    if (getHandleRect(selectionRect.topLeft(), scaleFactor).adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos))
        return State::ResizingTopLeft;
    if (getHandleRect(selectionRect.topRight(), scaleFactor).adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos))
        return State::ResizingTopRight;
    if (getHandleRect(selectionRect.bottomLeft(), scaleFactor).adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos))
        return State::ResizingBottomLeft;
    if (getHandleRect(selectionRect.bottomRight(), scaleFactor).adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos))
        return State::ResizingBottomRight;

    // Verificar lados
    if (getHandleRect(QPoint(selectionRect.center().x(), selectionRect.top()), scaleFactor).adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos))
        return State::ResizingTop;
    if (getHandleRect(QPoint(selectionRect.center().x(), selectionRect.bottom()), scaleFactor).adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos))
        return State::ResizingBottom;
    if (getHandleRect(QPoint(selectionRect.left(), selectionRect.center().y()), scaleFactor).adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos))
        return State::ResizingLeft;
    if (getHandleRect(QPoint(selectionRect.right(), selectionRect.center().y()), scaleFactor).adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos))
        return State::ResizingRight;

    return State::Selected;
}

QRect CropTool::getHandleRect(const QPoint &center, double scaleFactor) const {
    int size = handleSize;
    return QRect(center.x() - size/2, center.y() - size/2, size, size);
}

void CropTool::moveSelection(const QPoint &delta) {
    selectionRect.translate(delta);
    selectionRect = constrainToImageBounds(selectionRect);
}

void CropTool::resizeSelection(const QPoint &delta, State resizeState) {
    QRect newRect = selectionRect;

    switch (resizeState) {
        case State::ResizingTopLeft:
            newRect.setTopLeft(newRect.topLeft() + delta);
            break;
        case State::ResizingTopRight:
            newRect.setTopRight(newRect.topRight() + delta);
            break;
        case State::ResizingBottomLeft:
            newRect.setBottomLeft(newRect.bottomLeft() + delta);
            break;
        case State::ResizingBottomRight:
            newRect.setBottomRight(newRect.bottomRight() + delta);
            break;
        case State::ResizingTop:
            newRect.setTop(newRect.top() + delta.y());
            break;
        case State::ResizingBottom:
            newRect.setBottom(newRect.bottom() + delta.y());
            break;
        case State::ResizingLeft:
            newRect.setLeft(newRect.left() + delta.x());
            break;
        case State::ResizingRight:
            newRect.setRight(newRect.right() + delta.x());
            break;
        default:
            break;
    }

    // Validar tamanho mínimo
    newRect = newRect.normalized();
    if (newRect.width() >= 10 && newRect.height() >= 10) {
        selectionRect = constrainToImageBounds(newRect);
    }
}

QCursor CropTool::getCursorForState(State state) const {
    switch (state) {
        case State::ResizingTopLeft:
        case State::ResizingBottomRight:
            return Qt::SizeFDiagCursor;
        case State::ResizingTopRight:
        case State::ResizingBottomLeft:
            return Qt::SizeBDiagCursor;
        case State::ResizingTop:
        case State::ResizingBottom:
            return Qt::SizeVerCursor;
        case State::ResizingLeft:
        case State::ResizingRight:
            return Qt::SizeHorCursor;
        case State::Moving:
        case State::MovingWithMouseOnly:
            return Qt::SizeAllCursor;
        default:
            return Qt::CrossCursor;
    }
}

void CropTool::drawDiamondHandle(QPainter &painter, const QPoint &center, int size) {
    // Desenhar losango (diamante) rotacionado 45 graus
    QPolygon diamond;
    int half = size / 2;
    
    diamond << QPoint(center.x(), center.y() - half)        // Topo
            << QPoint(center.x() + half, center.y())        // Direita
            << QPoint(center.x(), center.y() + half)        // Baixo
            << QPoint(center.x() - half, center.y());       // Esquerda
    
    painter.drawPolygon(diamond);
}

void CropTool::setMovingWithMouseMode(bool enabled) {
    if (enabled && state == State::Selected) {
        state = State::MovingWithMouseOnly;
        // Inicializar lastMousePos com posição atual do mouse
        lastMousePos = QCursor::pos();
    } else if (!enabled && state == State::MovingWithMouseOnly) {
        state = State::Selected;
    }
}

bool CropTool::handleKeyPress(QKeyEvent *event) {
    // ESC cancela modo MovingWithMouseOnly
    if (event->key() == Qt::Key_Escape) {
        if (state == State::MovingWithMouseOnly) {
            state = State::Selected;
            return true;
        }
    }
    return false;
}

QRect CropTool::constrainToImageBounds(const QRect &rect) const {
    if (imageSize.isEmpty()) {
        return rect;  // Sem limites definidos
    }
    
    // Limitar cada canto do retângulo aos limites da imagem
    int left = qMax(0, rect.left());
    int top = qMax(0, rect.top());
    int right = qMin(imageSize.width() - 1, rect.right());
    int bottom = qMin(imageSize.height() - 1, rect.bottom());
    
    // Garantir que o retângulo ainda é válido
    if (right < left) right = left;
    if (bottom < top) bottom = top;
    
    return QRect(QPoint(left, top), QPoint(right, bottom));
}
