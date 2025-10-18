#ifndef IMAGEPROPERTIESDIALOG_H
#define IMAGEPROPERTIESDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QFileInfo>
#include "../core/ProjectModel.h"

namespace FingerprintEnhancer {

/**
 * @brief Diálogo para editar propriedades de uma imagem digital
 */
class ImagePropertiesDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImagePropertiesDialog(FingerprintImage* image, QWidget *parent = nullptr);
    ~ImagePropertiesDialog() = default;

    QString getComments() const { return commentsEdit->toPlainText(); }
    int getDisplayNumber() const;
    QString getDisplayName() const;

private:
    void setupUI();
    void loadData();
    void validateNumber();

    FingerprintImage* image;
    
    // Widgets informativos (read-only)
    QLabel *filePathLabel;
    QLabel *imageSizeLabel;
    QLabel *fragmentCountLabel;
    QLabel *minutiaeCountLabel;
    QLabel *createdAtLabel;
    QLabel *modifiedAtLabel;
    QLabel *hashLabel;
    QLabel *uuidLabel;
    QLabel *rotationLabel;
    QLabel *transformsLabel;
    
    // Widgets editáveis
    QSpinBox *numberSpinBox;
    QLineEdit *nameEdit;
    QTextEdit *commentsEdit;
    
    QDialogButtonBox *buttonBox;
};

} // namespace FingerprintEnhancer

#endif // IMAGEPROPERTIESDIALOG_H
