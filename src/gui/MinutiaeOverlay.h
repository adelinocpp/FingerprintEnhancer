#ifndef MINUTIAEOVERLAY_H
#define MINUTIAEOVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QPoint>
#include "../core/ProjectModel.h"
#include "MinutiaeDisplayDialog.h"

namespace FingerprintEnhancer {

/**
 * Estados de edição interativa de minúcias
 */
enum class MinutiaEditState {
    IDLE,              // Nenhuma minúcia selecionada
    SELECTED,          // Minúcia selecionada (pronta para edição)
    EDITING_POSITION,  // Arrastando para mover posição
    EDITING_ANGLE      // Arrastando para rotacionar ângulo
};

/**
 * Widget de overlay para desenhar minúcias sobre a imagem
 * Não modifica a imagem original - desenha em uma camada separada
 */
class MinutiaeOverlay : public QWidget {
    Q_OBJECT

public:
    explicit MinutiaeOverlay(QWidget *parent = nullptr);

    void setFragment(Fragment* fragment);
    void setScaleFactor(double scale);
    void setScrollOffset(const QPoint& offset);
    void setImageOffset(const QPoint& offset);  // Offset de centralização da imagem
    void clearMinutiae();

    // Controle de seleção
    void setSelectedMinutia(const QString& minutiaId);
    QString getSelectedMinutiaId() const { return selectedMinutiaId; }
    void clearSelection();

    // Modo de edição interativa
    void setEditMode(bool enabled) { 
        editMode = enabled; 
        if (!enabled) {
            editState = MinutiaEditState::IDLE;
            clearSelection();
        }
        update(); 
    }
    bool isEditMode() const { return editMode; }
    MinutiaEditState getEditState() const { return editState; }
    
    // Controle de modo de edição
    void setEditingPosition() { editState = MinutiaEditState::EDITING_POSITION; emit editStateChanged(editState); update(); }
    void setEditingAngle() { editState = MinutiaEditState::EDITING_ANGLE; emit editStateChanged(editState); update(); }

    // Configurações visuais
    void setShowLabels(bool show) { showLabels = show; update(); }
    void setShowAngles(bool show) { showAngles = show; update(); }
    void setMinutiaRadius(int radius) { minutiaRadius = radius; update(); }
    void setDisplaySettings(const MinutiaeDisplaySettings& settings) {
        displaySettings = settings;
        update();
    }

signals:
    void minutiaClicked(const QString& minutiaId, const QPoint& position);
    void minutiaDoubleClicked(const QString& minutiaId);
    void positionChanged(const QString& minutiaId, const QPoint& newPosition);
    void angleChanged(const QString& minutiaId, float newAngle);
    void editStateChanged(MinutiaEditState newState);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    Fragment* currentFragment;
    double scaleFactor;
    QPoint scrollOffset;
    QPoint imageOffset;  // Offset de centralização da imagem
    QString selectedMinutiaId;
    bool editMode;
    MinutiaEditState editState;
    bool isDragging;
    QPoint dragStartPos;
    QPoint lastDragPos;
    float initialAngle;

    // Configurações visuais
    bool showLabels;
    bool showAngles;
    int minutiaRadius;
    MinutiaeDisplaySettings displaySettings;

    // Cores
    QColor normalColor;
    QColor selectedColor;
    QColor hoverColor;

    // Helper functions
    void drawMinutia(QPainter& painter, const Minutia& minutia, bool isSelected);
    void drawMinutiaWithArrow(QPainter& painter, const QPoint& pos, float angle, const QColor& color, bool isSelected);
    void drawMinutiaLabel(QPainter& painter, const QPoint& pos, int number, const QString& type, bool isSelected);
    void drawEditStateIndicator(QPainter& painter, const QPoint& pos);

    Minutia* findMinutiaAt(const QPoint& pos);
    QPoint scalePoint(const QPoint& imagePoint) const;
    QPoint unscalePoint(const QPoint& widgetPoint) const;
    float calculateAngleFromDrag(const QPoint& center, const QPoint& dragPos) const;
};

} // namespace FingerprintEnhancer

#endif // MINUTIAEOVERLAY_H
