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

class RotationDialog : public QDialog {
    Q_OBJECT

public:
    explicit RotationDialog(const cv::Mat &image, ImageViewer *viewer, QWidget *parent = nullptr);
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

    double finalAngle;
    bool accepted;
    bool useTransparentBackground;

    QSlider *angleSlider;
    QDoubleSpinBox *angleSpinBox;
    QCheckBox *transparentBgCheckbox;
    QLabel *previewLabel;
    QPushButton *acceptButton;
    QPushButton *cancelButton;

    void updatePreview(double angle);
    cv::Mat rotateImage(const cv::Mat &image, double angle);
};

#endif // ROTATIONDIALOG_H
