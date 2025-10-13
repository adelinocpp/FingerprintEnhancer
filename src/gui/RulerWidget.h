#ifndef RULERWIDGET_H
#define RULERWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QColor>

/**
 * @brief Widget de régua métrica para os painéis de visualização
 * 
 * Exibe uma régua com marcações em milímetros baseada na escala calibrada.
 * Pode ser posicionada horizontal ou verticalmente nas laterais dos painéis.
 */
class RulerWidget : public QWidget {
    Q_OBJECT

public:
    enum Orientation {
        HORIZONTAL,  // Régua na parte superior/inferior
        VERTICAL     // Régua na lateral esquerda/direita
    };

    enum Position {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT
    };

    explicit RulerWidget(Orientation orientation, Position position, QWidget *parent = nullptr);
    ~RulerWidget();

    // Configuração
    void setOrientation(Orientation orientation);
    void setPosition(Position position);
    void setScale(double pixelsPerMM);  // Escala calibrada
    void setZoomFactor(double factor);
    void setScrollOffset(int offset);   // Offset do scroll
    void setImageOffset(int offset);    // Offset da imagem centralizada
    void setVisible(bool visible);

    // Parâmetros visuais
    void setBackgroundColor(const QColor& color);
    void setTextColor(const QColor& color);
    void setLineColor(const QColor& color);
    void setRulerWidth(int width);

    // Estado
    bool isVisible() const { return visible; }
    double getScale() const { return scale; }
    Orientation getOrientation() const { return orientation; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // Configuração
    Orientation orientation;
    Position position;
    double scale;           // pixels/mm
    double zoomFactor;
    int scrollOffset;
    int imageOffset;
    bool visible;
    int rulerWidth;         // Largura/altura da régua

    // Cores
    QColor backgroundColor;
    QColor textColor;
    QColor lineColor;

    // Métodos auxiliares
    void drawHorizontalRuler(QPainter& painter);
    void drawVerticalRuler(QPainter& painter);
    void drawTick(QPainter& painter, int pos, int length, bool isMain);
    void drawLabel(QPainter& painter, int pos, const QString& label, bool isHorizontal);
    int calculateTickSpacing() const;
    QString formatDistance(double mm) const;
};

#endif // RULERWIDGET_H
