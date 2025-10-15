#ifndef CROPTOOL_H
#define CROPTOOL_H

#include <QWidget>
#include <QRect>
#include <QPoint>
#include <QPainter>
#include <QMouseEvent>
#include <opencv2/opencv.hpp>

/**
 * @brief Ferramenta de seleção e recorte de região em imagens
 *
 * Permite ao usuário selecionar uma região retangular via mouse
 * e recortar a imagem para essa área.
 */
class CropTool : public QObject {
    Q_OBJECT

public:
    enum class State {
        Inactive,
        Selecting,
        Selected,
        Moving,
        MovingWithMouseOnly,  // Modo movimento apenas com mouse (sem clicar)
        ResizingTopLeft,
        ResizingTopRight,
        ResizingBottomLeft,
        ResizingBottomRight,
        ResizingTop,
        ResizingBottom,
        ResizingLeft,
        ResizingRight
    };

    explicit CropTool(QObject *parent = nullptr);
    ~CropTool();

    // Controle de ativação
    void setActive(bool active);
    bool isActive() const { return state != State::Inactive; }

    // Configurar área de seleção
    void setSelectionRect(const QRect &rect);
    QRect getSelectionRect() const { return selectionRect; }
    bool hasSelection() const { return state == State::Selected || state == State::Moving; }

    // Limpar seleção
    void clearSelection();

    // Desenhar overlay de seleção
    void draw(QPainter &painter, double scaleFactor = 1.0);

    // Processar eventos de mouse
    bool handleMousePress(QMouseEvent *event, double scaleFactor);
    bool handleMouseMove(QMouseEvent *event, double scaleFactor);
    bool handleMouseRelease(QMouseEvent *event, double scaleFactor);
    
    // Processar eventos de teclado
    bool handleKeyPress(QKeyEvent *event);

    // Modo movimento apenas com mouse
    void setMovingWithMouseMode(bool enabled);
    bool isMovingWithMouseMode() const { return state == State::MovingWithMouseOnly; }

    // Recortar imagem
    cv::Mat cropImage(const cv::Mat &image) const;

signals:
    void selectionChanged(QRect selection);
    void selectionFinished(QRect selection);

private:
    State state;
    QRect selectionRect;
    QPoint startPoint;
    QPoint currentPoint;
    QPoint lastMousePos;

    // Configurações visuais
    QColor selectionBorderColor;
    QColor selectionFillColor;
    QColor handleColor;
    int handleSize;
    int borderWidth;

    // Métodos auxiliares
    State getResizeState(const QPoint &pos, double scaleFactor) const;
    QRect getHandleRect(const QPoint &center, double scaleFactor) const;
    void drawDiamondHandle(QPainter &painter, const QPoint &center, int size);
    void updateSelectionFromDrag(const QPoint &current);
    void moveSelection(const QPoint &delta);
    void resizeSelection(const QPoint &delta, State resizeState);
    QCursor getCursorForState(State state) const;
};

#endif // CROPTOOL_H
