#ifndef ROTATIONDIALOG_H
#define ROTATIONDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <opencv2/opencv.hpp>

class ImageViewer;
class FragmentRegionsOverlay;

namespace FingerprintEnhancer {
    struct Fragment;
    class MinutiaeOverlay;
    struct Minutia;
    class FingerprintImage;
}

class RotationDialog : public QDialog {
    Q_OBJECT

public:
    explicit RotationDialog(const cv::Mat &image, ImageViewer *viewer, 
                           FingerprintEnhancer::Fragment *fragment = nullptr,
                           FingerprintEnhancer::MinutiaeOverlay *overlay = nullptr,
                           FingerprintEnhancer::FingerprintImage *parentImage = nullptr,
                           FragmentRegionsOverlay *fragmentOverlay = nullptr,
                           QWidget *parent = nullptr);
    ~RotationDialog();

    double getRotationAngle() const { return finalAngle; }
    cv::Mat getRotatedImage() const { return rotatedImage; }
    bool wasAccepted() const { return accepted; }

private slots:
    void onSliderChanged(int value);
    void onSpinBoxChanged(double value);
    void onAccept();
    void onReject();

private slots:
    void onTransparentBgChanged(int state);

private:
    cv::Mat originalImage;
    cv::Mat rotatedImage;
    ImageViewer *imageViewer;
    FingerprintEnhancer::Fragment *currentFragment;
    FingerprintEnhancer::MinutiaeOverlay *minutiaeOverlay;
    FingerprintEnhancer::FingerprintImage *parentImage;
    FragmentRegionsOverlay *fragmentRegionsOverlay;

    double finalAngle;
    bool accepted;
    bool useTransparentBackground;
    
    // Backup das minúcias originais para restaurar no cancelamento
    QList<FingerprintEnhancer::Minutia> originalMinutiae;

    QSlider *angleSlider;
    QDoubleSpinBox *angleSpinBox;
    QCheckBox *transparentBgCheckbox;
    QLabel *previewLabel;
    QLabel *currentAngleLabel;  // Exibe ângulo atual do objeto
    QPushButton *acceptButton;
    QPushButton *cancelButton;

    void updatePreview(double angle);
    cv::Mat rotateImage(const cv::Mat &image, double angle);
};

#endif // ROTATIONDIALOG_H
