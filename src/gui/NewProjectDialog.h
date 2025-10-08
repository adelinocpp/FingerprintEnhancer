#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace FingerprintEnhancer {

/**
 * Diálogo para criar novo projeto
 */
class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);

    QString getProjectName() const;
    QString getCaseNumber() const;
    QString getDescription() const;

private slots:
    void onAccept();
    void validateInput();

private:
    QLineEdit* projectNameEdit;
    QLineEdit* caseNumberEdit;
    QTextEdit* descriptionEdit;
    QPushButton* okButton;

    void setupUI();
};

} // namespace FingerprintEnhancer

#endif // NEWPROJECTDIALOG_H
