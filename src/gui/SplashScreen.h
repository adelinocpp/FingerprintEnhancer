#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>
#include <QPixmap>
#include <QPainter>
#include <QApplication>

/**
 * @brief Tela de splash exibida durante inicialização
 */
class SplashScreen : public QSplashScreen {
    Q_OBJECT

public:
    explicit SplashScreen();
    ~SplashScreen() = default;
    
    void showMessage(const QString& message, int alignment = Qt::AlignBottom | Qt::AlignCenter);

private:
    QPixmap createSplashPixmap();
};

#endif // SPLASHSCREEN_H
