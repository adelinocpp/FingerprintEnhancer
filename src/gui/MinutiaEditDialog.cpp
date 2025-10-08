#include "MinutiaEditDialog.h"
#include <QStandardItemModel>

namespace FingerprintEnhancer {

MinutiaEditDialog::MinutiaEditDialog(Minutia* minutia, QWidget *parent)
    : QDialog(parent), currentMinutia(minutia)
{
    setWindowTitle("Editar Minúcia");
    setupUI();
    loadMinutiaData();
}

void MinutiaEditDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Informações da minúcia
    infoLabel = new QLabel(this);
    infoLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 8px; border-radius: 4px; }");
    mainLayout->addWidget(infoLabel);

    // Grupo de posição
    QGroupBox* positionGroup = new QGroupBox("Posição", this);
    QFormLayout* positionLayout = new QFormLayout(positionGroup);

    xSpinBox = new QSpinBox(this);
    xSpinBox->setRange(0, 10000);
    xSpinBox->setSuffix(" px");
    positionLayout->addRow("X:", xSpinBox);

    ySpinBox = new QSpinBox(this);
    ySpinBox->setRange(0, 10000);
    ySpinBox->setSuffix(" px");
    positionLayout->addRow("Y:", ySpinBox);

    mainLayout->addWidget(positionGroup);

    // Grupo de classificação
    QGroupBox* classificationGroup = new QGroupBox("Classificação", this);
    QFormLayout* classificationLayout = new QFormLayout(classificationGroup);

    typeComboBox = new QComboBox(this);
    populateTypeComboBox();
    classificationLayout->addRow("Tipo:", typeComboBox);

    angleSpinBox = new QDoubleSpinBox(this);
    angleSpinBox->setRange(0.0, 360.0);
    angleSpinBox->setSuffix("°");
    angleSpinBox->setDecimals(1);
    classificationLayout->addRow("Ângulo:", angleSpinBox);

    qualitySpinBox = new QDoubleSpinBox(this);
    qualitySpinBox->setRange(0.0, 1.0);
    qualitySpinBox->setSingleStep(0.1);
    qualitySpinBox->setDecimals(2);
    classificationLayout->addRow("Qualidade:", qualitySpinBox);

    mainLayout->addWidget(classificationGroup);

    // Observações
    QGroupBox* notesGroup = new QGroupBox("Observações", this);
    QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);

    notesTextEdit = new QTextEdit(this);
    notesTextEdit->setMaximumHeight(100);
    notesLayout->addWidget(notesTextEdit);

    mainLayout->addWidget(notesGroup);

    // Botões
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* cancelButton = new QPushButton("Cancelar", this);
    connect(cancelButton, &QPushButton::clicked, this, &MinutiaEditDialog::onReject);
    buttonLayout->addWidget(cancelButton);

    QPushButton* okButton = new QPushButton("OK", this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &MinutiaEditDialog::onAccept);
    buttonLayout->addWidget(okButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    resize(400, 500);
}

void MinutiaEditDialog::populateTypeComboBox() {
    // Adicionar tipos de minúcias organizados por categoria
    typeComboBox->addItem("Não classificada", static_cast<int>(MinutiaeType::OTHER));

    // Core e Delta
    typeComboBox->addItem("--- Pontos Singulares ---", -1);
    typeComboBox->addItem("Core (Núcleo)", static_cast<int>(MinutiaeType::CORE));
    typeComboBox->addItem("Delta", static_cast<int>(MinutiaeType::DELTA));

    // Tipos básicos
    typeComboBox->addItem("--- Tipos Básicos ---", -1);
    typeComboBox->addItem("Ridge Ending B (Final de crista B)", static_cast<int>(MinutiaeType::RIDGE_ENDING_B));
    typeComboBox->addItem("Ridge Ending C (Final de crista C)", static_cast<int>(MinutiaeType::RIDGE_ENDING_C));
    typeComboBox->addItem("Bifurcation (Bifurcação)", static_cast<int>(MinutiaeType::BIFURCATION));

    // Bifurcações
    typeComboBox->addItem("--- Bifurcações ---", -1);
    typeComboBox->addItem("BTUS (Bifurcação tipo U superior)", static_cast<int>(MinutiaeType::BTUS));
    typeComboBox->addItem("BTUI (Bifurcação tipo U inferior)", static_cast<int>(MinutiaeType::BTUI));
    typeComboBox->addItem("BTBS (Bifurcação tipo B superior)", static_cast<int>(MinutiaeType::BTBS));
    typeComboBox->addItem("BTBI (Bifurcação tipo B inferior)", static_cast<int>(MinutiaeType::BTBI));

    // Convergências
    typeComboBox->addItem("--- Convergências ---", -1);
    typeComboBox->addItem("Convergence (Convergência)", static_cast<int>(MinutiaeType::CONVERGENCE));
    typeComboBox->addItem("CTUS (Convergência tipo U superior)", static_cast<int>(MinutiaeType::CTUS));
    typeComboBox->addItem("CTUI (Convergência tipo U inferior)", static_cast<int>(MinutiaeType::CTUI));
    typeComboBox->addItem("CTCS (Convergência tipo C superior)", static_cast<int>(MinutiaeType::CTCS));
    typeComboBox->addItem("CTCI (Convergência tipo C inferior)", static_cast<int>(MinutiaeType::CTCI));

    // Outros tipos complexos
    typeComboBox->addItem("--- Tipos Complexos ---", -1);
    typeComboBox->addItem("Fragmento Pequeno", static_cast<int>(MinutiaeType::FRAGMENT_SMALL));
    typeComboBox->addItem("Fragmento Grande", static_cast<int>(MinutiaeType::FRAGMENT_LARGE));
    typeComboBox->addItem("Bridge B (Ponte B)", static_cast<int>(MinutiaeType::BRIDGE_B));
    typeComboBox->addItem("Bridge C (Ponte C)", static_cast<int>(MinutiaeType::BRIDGE_C));
    typeComboBox->addItem("Tridente", static_cast<int>(MinutiaeType::TRIPOD));

    // Desabilitar itens de categoria (separadores)
    for (int i = 0; i < typeComboBox->count(); ++i) {
        if (typeComboBox->itemData(i).toInt() == -1) {
            const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(typeComboBox->model());
            if (model) {
                QStandardItem* item = model->item(i);
                if (item) {
                    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
                    item->setData(QFont("Arial", 10, QFont::Bold), Qt::FontRole);
                }
            }
        }
    }
}

void MinutiaEditDialog::loadMinutiaData() {
    if (!currentMinutia) return;

    // Atualizar label de informações
    QString info = QString("ID: %1 | Criada: %2 | Modificada: %3")
        .arg(currentMinutia->id)
        .arg(currentMinutia->createdAt.toString("dd/MM/yyyy hh:mm"))
        .arg(currentMinutia->modifiedAt.toString("dd/MM/yyyy hh:mm"));
    infoLabel->setText(info);

    // Carregar dados
    xSpinBox->setValue(currentMinutia->position.x());
    ySpinBox->setValue(currentMinutia->position.y());
    angleSpinBox->setValue(currentMinutia->angle);
    qualitySpinBox->setValue(currentMinutia->quality);
    notesTextEdit->setPlainText(currentMinutia->notes);

    // Selecionar tipo
    int typeValue = static_cast<int>(currentMinutia->type);
    for (int i = 0; i < typeComboBox->count(); ++i) {
        if (typeComboBox->itemData(i).toInt() == typeValue) {
            typeComboBox->setCurrentIndex(i);
            break;
        }
    }
}

void MinutiaEditDialog::onAccept() {
    if (currentMinutia) {
        currentMinutia->position = getPosition();
        currentMinutia->type = getType();
        currentMinutia->angle = getAngle();
        currentMinutia->quality = getQuality();
        currentMinutia->notes = getNotes();
        currentMinutia->modifiedAt = QDateTime::currentDateTime();
    }
    accept();
}

void MinutiaEditDialog::onReject() {
    reject();
}

QPoint MinutiaEditDialog::getPosition() const {
    return QPoint(xSpinBox->value(), ySpinBox->value());
}

MinutiaeType MinutiaEditDialog::getType() const {
    int typeValue = typeComboBox->currentData().toInt();
    if (typeValue == -1) return MinutiaeType::OTHER; // Categoria, não tipo válido
    return static_cast<MinutiaeType>(typeValue);
}

float MinutiaEditDialog::getAngle() const {
    return static_cast<float>(angleSpinBox->value());
}

float MinutiaEditDialog::getQuality() const {
    return static_cast<float>(qualitySpinBox->value());
}

QString MinutiaEditDialog::getNotes() const {
    return notesTextEdit->toPlainText();
}

} // namespace FingerprintEnhancer
