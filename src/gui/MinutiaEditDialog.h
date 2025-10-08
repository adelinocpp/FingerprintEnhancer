#ifndef MINUTIAEDITDIALOG_H
#define MINUTIAEDITDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include "../core/ProjectModel.h"
#include "../core/MinutiaeTypes.h"

namespace FingerprintEnhancer {

/**
 * Diálogo para editar propriedades de uma minúcia
 * Permite alterar: posição, tipo, ângulo, qualidade e observações
 */
class MinutiaEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit MinutiaEditDialog(Minutia* minutia, QWidget *parent = nullptr);

    // Getters para valores editados
    QPoint getPosition() const;
    MinutiaeType getType() const;
    float getAngle() const;
    float getQuality() const;
    QString getNotes() const;

private slots:
    void onAccept();
    void onReject();

private:
    Minutia* currentMinutia;

    // Widgets de edição
    QSpinBox* xSpinBox;
    QSpinBox* ySpinBox;
    QComboBox* typeComboBox;
    QDoubleSpinBox* angleSpinBox;
    QDoubleSpinBox* qualitySpinBox;
    QTextEdit* notesTextEdit;

    QLabel* infoLabel;

    void setupUI();
    void populateTypeComboBox();
    void loadMinutiaData();
};

} // namespace FingerprintEnhancer

#endif // MINUTIAEDITDIALOG_H
