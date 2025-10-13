#ifndef SCALECALIBRATIONTOOL_H
#define SCALECALIBRATIONTOOL_H

#include <QWidget>
#include <QPoint>
#include <QVector>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>

/**
 * @brief Ferramenta interativa para calibração de escala baseada em distância entre cristas
 * 
 * Inspirada no PrintQuest AFIS - permite ao usuário desenhar uma linha
 * atravessando múltiplas cristas da impressão digital para calibrar a escala.
 * 
 * Distância típica entre cristas: 0.4-0.5mm (500 microns)
 */
class ScaleCalibrationTool : public QWidget {
    Q_OBJECT

public:
    enum CalibrationState {
        STATE_IDLE,              // Aguardando início
        STATE_DRAWING_LINE,      // Desenhando linha de medição
        STATE_COUNTING_RIDGES,   // Contando cristas
        STATE_COMPLETED          // Calibração concluída
    };

    explicit ScaleCalibrationTool(QWidget *parent = nullptr);
    ~ScaleCalibrationTool();

    // Controle da ferramenta
    void activate();
    void deactivate();
    bool isActive() const { return active; }
    void reset();

    // Configuração
    void setImage(const QImage& image);
    void setImageSize(const QSize& size);
    void setZoomFactor(double factor);
    void setScrollOffset(const QPoint& offset);
    void setImageOffset(const QPoint& offset);

    // Resultados
    double getPixelDistance() const { return pixelDistance; }
    int getRidgeCount() const { return ridgeCount; }
    double getCalculatedScale() const; // pixels/mm
    bool hasValidCalibration() const;

    // Parâmetros
    void setDefaultRidgeSpacing(double mm) { defaultRidgeSpacing = mm; }
    double getDefaultRidgeSpacing() const { return defaultRidgeSpacing; }

signals:
    void calibrationStarted();
    void lineDrawn(QPoint start, QPoint end, double pixelDistance);
    void ridgeCountChanged(int count);
    void calibrationCompleted(double scale, double confidence);
    void calibrationCancelled();
    void stateChanged(CalibrationState state);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    // Estado
    bool active;
    CalibrationState currentState;

    // Geometria
    QImage backgroundImage;
    QSize imageSize;
    double zoomFactor;
    QPoint scrollOffset;
    QPoint imageOffset;

    // Linha de medição
    QPoint lineStart;      // Coordenadas da imagem
    QPoint lineEnd;        // Coordenadas da imagem
    QPoint currentPos;     // Posição atual do mouse
    bool isDrawing;
    double pixelDistance;

    // Contagem de cristas
    int ridgeCount;
    QVector<QPoint> ridgeMarkers;  // Pontos marcados pelo usuário

    // Parâmetros
    double defaultRidgeSpacing;  // 0.5mm típico

    // Métodos auxiliares
    QPoint imageToWidget(const QPoint& imagePoint) const;
    QPoint widgetToImage(const QPoint& widgetPoint) const;
    double calculateDistance(const QPoint& p1, const QPoint& p2) const;
    int findNearestRidgeMarker(const QPoint& pos, double maxDistance = 15.0) const;
    void removeRidgeMarker(int index);
    void drawCalibrationLine(QPainter& painter);
    void drawRidgeMarkers(QPainter& painter);
    void drawInstructions(QPainter& painter);
    void drawResultsPanel(QPainter& painter);
    void autoDetectRidges();
    double estimateConfidence() const;
};

#endif // SCALECALIBRATIONTOOL_H
