#ifndef FFTFILTERDIALOG_H
#define FFTFILTERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QRect>
#include <QMouseEvent>
#include <opencv2/opencv.hpp>

class ImageViewer;

/**
 * @brief Widget personalizado para exibir o espectro FFT e permitir desenho de máscaras
 */
class FFTSpectrumLabel : public QLabel {
    Q_OBJECT

public:
    explicit FFTSpectrumLabel(QWidget *parent = nullptr);

    void setSpectrum(const cv::Mat &spectrum);
    void addMaskRect(const QRect &rect);
    void clearMasks();
    QVector<QRect> getMaskRects() const { return maskRects; }
    void setInvertMask(bool invert) { invertMask = invert; update(); }

signals:
    void maskChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPixmap spectrumPixmap;
    QVector<QRect> maskRects;
    QPoint dragStart;
    QRect currentRect;
    bool isDrawing;
    bool invertMask;
};

/**
 * @brief Diálogo interativo para filtrar imagem usando FFT
 *
 * Permite visualizar o espectro de frequências e selecionar
 * regiões para remover (ou manter) frequências indesejadas.
 */
class FFTFilterDialog : public QDialog {
    Q_OBJECT

public:
    explicit FFTFilterDialog(const cv::Mat &image, QWidget *parent = nullptr);
    ~FFTFilterDialog();

    cv::Mat getFilteredImage() const { return filteredImage; }
    bool wasAccepted() const { return accepted; }

private slots:
    void onMaskChanged();
    void onInvertMaskChanged(int state);
    void onClearMasks();
    void onAccept();
    void onReject();

private:
    // Imagens
    cv::Mat originalImage;
    cv::Mat filteredImage;
    cv::Mat complexFFT;
    cv::Mat magnitudeSpectrum;

    // Estado
    bool accepted;
    bool invertMask;

    // Widgets
    FFTSpectrumLabel *spectrumLabel;
    ImageViewer *previewViewer;
    QCheckBox *invertMaskCheckbox;
    QPushButton *clearButton;
    QPushButton *acceptButton;
    QPushButton *cancelButton;

    // Métodos auxiliares
    void computeFFT();
    void computeMagnitudeSpectrum();
    void updatePreview();
    cv::Mat createMaskFromRects(const QVector<QRect> &rects, const cv::Size &size);
    cv::Mat applyFFTMask(const cv::Mat &complexImg, const cv::Mat &mask);
    cv::Mat shiftFFT(const cv::Mat &img);
};

#endif // FFTFILTERDIALOG_H
