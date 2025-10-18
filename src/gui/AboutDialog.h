#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>

/**
 * @brief Diálogo "Sobre" com informações do programa e contato
 */
class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() = default;

private:
    void setupUI();
    QString getAboutText() const;
};

#endif // ABOUTDIALOG_H
