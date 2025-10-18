#ifndef FRAGMENTPROPERTIESDIALOG_H
#define FRAGMENTPROPERTIESDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include "../core/ProjectModel.h"

namespace FingerprintEnhancer {

/**
 * @brief Diálogo para editar propriedades de um fragmento
 */
class FragmentPropertiesDialog : public QDialog {
    Q_OBJECT

public:
    explicit FragmentPropertiesDialog(Fragment* fragment, QWidget *parent = nullptr);
    ~FragmentPropertiesDialog() = default;

    QString getComments() const { return commentsEdit->toPlainText(); }
    double getPixelsPerMM() const;
    int getDisplayNumber() const;
    QString getDisplayName() const;

private:
    void setupUI();
    void loadData();
    void saveData();
    void updateDpiFromScale();
    void updateScaleFromDpi();
    void validateNumber();

    Fragment* fragment;
    
    // Widgets informativos (read-only)
    QLabel *fragmentIdLabel;
    QLabel *parentImageLabel;
    QLabel *sourceRectLabel;
    QLabel *imageSizeLabel;
    QLabel *minutiaeCountLabel;
    QLabel *createdAtLabel;
    QLabel *modifiedAtLabel;
    QLabel *uuidLabel;
    QLabel *rotationLabel;
    QLabel *transformsLabel;
    
    // Widgets editáveis
    QSpinBox *numberSpinBox;         // Número do fragmento (apenas segunda parte)
    QLineEdit *nameEdit;
    QTextEdit *commentsEdit;
    QDoubleSpinBox *scaleSpinBox;  // Escala em pixels/mm
    QDoubleSpinBox *dpiSpinBox;    // Escala em DPI
    
    QDialogButtonBox *buttonBox;
    
private slots:
    void onScaleChanged(double value);
    void onDpiChanged(double value);
};

} // namespace FingerprintEnhancer

#endif // FRAGMENTPROPERTIESDIALOG_H
