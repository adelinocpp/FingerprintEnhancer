#ifndef FRAGMENTPROPERTIESDIALOG_H
#define FRAGMENTPROPERTIESDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QFormLayout>
#include <QDialogButtonBox>
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

private:
    void setupUI();
    void loadData();

    Fragment* fragment;
    
    // Widgets informativos (read-only)
    QLabel *fragmentIdLabel;
    QLabel *sourceRectLabel;
    QLabel *imageSizeLabel;
    QLabel *minutiaeCountLabel;
    QLabel *createdAtLabel;
    QLabel *modifiedAtLabel;
    
    // Widgets editáveis
    QTextEdit *commentsEdit;
    
    QDialogButtonBox *buttonBox;
};

} // namespace FingerprintEnhancer

#endif // FRAGMENTPROPERTIESDIALOG_H
