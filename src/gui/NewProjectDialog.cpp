#include "NewProjectDialog.h"

namespace FingerprintEnhancer {

NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Novo Projeto");
    setupUI();
}

void NewProjectDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Formulário
    QFormLayout* formLayout = new QFormLayout();

    projectNameEdit = new QLineEdit(this);
    projectNameEdit->setPlaceholderText("Digite o nome do projeto");
    connect(projectNameEdit, &QLineEdit::textChanged, this, &NewProjectDialog::validateInput);
    formLayout->addRow("Nome do Projeto*:", projectNameEdit);

    caseNumberEdit = new QLineEdit(this);
    caseNumberEdit->setPlaceholderText("Número do caso (opcional)");
    formLayout->addRow("Número do Caso:", caseNumberEdit);

    descriptionEdit = new QTextEdit(this);
    descriptionEdit->setPlaceholderText("Descrição do projeto (opcional)");
    descriptionEdit->setMaximumHeight(100);
    formLayout->addRow("Descrição:", descriptionEdit);

    mainLayout->addLayout(formLayout);

    // Nota
    QLabel* noteLabel = new QLabel("* Campos obrigatórios", this);
    noteLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    mainLayout->addWidget(noteLabel);

    // Botões
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* cancelButton = new QPushButton("Cancelar", this);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);

    okButton = new QPushButton("Criar", this);
    okButton->setDefault(true);
    okButton->setEnabled(false);
    connect(okButton, &QPushButton::clicked, this, &NewProjectDialog::onAccept);
    buttonLayout->addWidget(okButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    resize(400, 300);
}

void NewProjectDialog::validateInput() {
    bool valid = !projectNameEdit->text().trimmed().isEmpty();
    okButton->setEnabled(valid);
}

void NewProjectDialog::onAccept() {
    if (projectNameEdit->text().trimmed().isEmpty()) {
        return;
    }
    accept();
}

QString NewProjectDialog::getProjectName() const {
    return projectNameEdit->text().trimmed();
}

QString NewProjectDialog::getCaseNumber() const {
    return caseNumberEdit->text().trimmed();
}

QString NewProjectDialog::getDescription() const {
    return descriptionEdit->toPlainText().trimmed();
}

} // namespace FingerprintEnhancer
