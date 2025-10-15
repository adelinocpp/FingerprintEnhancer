#ifndef PROJECTSAVERWORKER_H
#define PROJECTSAVERWORKER_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QTimer>

namespace FingerprintEnhancer {

/**
 * @brief Worker para salvar projeto em thread separada
 * 
 * Salva o projeto sem travar a interface, com timeout de segurança
 * e relatório de progresso
 */
class ProjectSaverWorker : public QThread {
    Q_OBJECT

public:
    explicit ProjectSaverWorker(QObject *parent = nullptr);
    ~ProjectSaverWorker();

    void setSaveParameters(const QString& projectPath);
    void run() override;
    void cancel();

    // Timeout em milissegundos (padrão: 5 minutos)
    void setTimeout(int milliseconds) { timeoutMs = milliseconds; }

signals:
    void progressUpdated(const QString& message);
    void saveCompleted(bool success, const QString& message);
    void saveTimeout();
    void finished();

private slots:
    void onTimeout();

private:
    QString projectPath;
    QMutex mutex;
    bool cancelled;
    int timeoutMs;
    QTimer* timeoutTimer;
};

} // namespace FingerprintEnhancer

#endif // PROJECTSAVERWORKER_H
