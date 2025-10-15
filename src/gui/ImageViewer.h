#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtCore/QPoint>
#include <opencv2/opencv.hpp>

// Forward declaration
class ImageViewer;

/**
 * @brief QLabel customizado que permite desenhar overlay sobre a imagem
 */
class CropOverlayLabel : public QLabel {
    Q_OBJECT

public:
    explicit CropOverlayLabel(ImageViewer *viewer, QWidget *parent = nullptr);
    void setOverlayRect(const QRect &rect);
    void clearOverlay();
    void setOverlayEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ImageViewer *imageViewer;
    QRect overlayRect;
    bool overlayEnabled;
};

/**
 * @brief Widget personalizado para visualização de imagens com zoom e pan
 *
 * Implementa funcionalidades avançadas de visualização necessárias para
 * análise forense de impressões digitais, incluindo zoom sincronizado
 * e marcação de pontos.
 */
class ImageViewer : public QScrollArea {
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer();
    
    // Carregamento e exibição de imagens
    void setImage(const cv::Mat &image);
    void setPixmap(const QPixmap &pixmap);
    void clearImage();
    
    // Controles de zoom
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomToActual();
    void setZoomFactor(double factor);
    double getZoomFactor() const { return scaleFactor; }
    
    // Sincronização com outro viewer
    void setSyncViewer(ImageViewer *viewer);
    void syncViewport(const QPoint &position);
    
    // Estado da imagem
    bool hasImage() const;
    cv::Mat getCurrentImage() const;
    QSize getImageSize() const;

    // Conversão de coordenadas
    QPoint imageToWidget(const QPoint &imagePoint) const;
    QPoint widgetToImage(const QPoint &widgetPoint) const;

    // Modo de seleção para recorte
    void setCropMode(bool enabled);
    bool isCropMode() const { return cropModeEnabled; }
    QRect getCropSelection() const { return cropSelection; }
    bool hasCropSelection() const { return !cropSelection.isNull(); }
    void setCropSelection(const QRect& rect);
    void clearCropSelection();

    // Acesso para CropOverlayLabel
    double getScaleFactor() const { return scaleFactor; }
    QRect getCropSelectionInternal() const { return cropSelection; }
    bool isCropModeEnabled() const { return cropModeEnabled; }

    // Calcular offset de centralização da imagem
    QPoint getImageOffset() const;

signals:
    void imageClicked(QPoint imagePosition);
    void mouseMoved(QPoint imagePosition);
    void zoomChanged(double factor);
    void viewportChanged(QPoint position);
    void scrollChanged(QPoint scrollOffset);
    void imageOffsetChanged(QPoint imageOffset);
    void cropSelectionChanged(QRect selection);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onSyncViewportChanged(QPoint position);
    void onScrollBarValueChanged();

private:
    // Componentes da interface
    CropOverlayLabel *imageLabel;

    // Estado da imagem
    cv::Mat currentImage;
    QPixmap currentPixmap;
    double scaleFactor;

    // Controle de pan
    bool panning;
    QPoint lastPanPoint;

    // Sincronização
    ImageViewer *syncViewer;
    bool syncEnabled;

    // Modo de recorte
    bool cropModeEnabled;
    bool isSelecting;
    bool isMovingSelection;  // Movendo seleção inteira (clicou dentro e está arrastando)
    bool isResizingEdge;     // Redimensionando borda específica
    QPoint cropStart;
    QPoint cropEnd;
    QRect cropSelection;
    
    // Estados de redimensionamento
    enum EdgeHandle {
        EDGE_NONE,
        EDGE_TOP,
        EDGE_BOTTOM,
        EDGE_LEFT,
        EDGE_RIGHT
    };
    EdgeHandle activeEdgeHandle;
    
    // Métodos auxiliares
    void updateImageDisplay();
    void scaleImage(double factor);
    QPixmap matToQPixmap(const cv::Mat &mat);
    void adjustScrollBars(double factor);
    void centerImage();
    
    // Detecção de handles de redimensionamento
    EdgeHandle getEdgeHandleAtPoint(const QPoint &widgetPos) const;
    bool isPointInDiamondHandle(const QPoint &widgetPos, const QPoint &handleCenter, int size) const;
    
    // Constantes
    static const double MIN_ZOOM_FACTOR;
    static const double MAX_ZOOM_FACTOR;
    static const double ZOOM_STEP;
};

#endif // IMAGEVIEWER_H

