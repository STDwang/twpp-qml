#include <QObject>
#include <QString>
#include <QDebug>
#include <QCamera>
#include <QCameraInfo>
#include <QAbstractVideoSurface>
#include "imageprovider.h"
#include "twglue.hpp"

class QtCameraCapture : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    enum PixelFormat {
        Format_Invalid,
        Format_ARGB32,
        Format_ARGB32_Premultiplied,
        Format_RGB32,
        Format_User = 1000
    };

    Q_ENUM(PixelFormat)
    explicit QtCameraCapture(QObject *parent = 0);
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;
    bool present(const QVideoFrame &frame) override;
signals:
    void frameAvailable(QImage frame);
};

class QtCamera : public QObject
{
    Q_OBJECT
public:
    explicit QtCamera(QCameraInfo cameraInfo,
                      QObject *parent,
                      const TwGlue& glue);
    ~QtCamera();

    Q_INVOKABLE bool start();
    Q_INVOKABLE bool stop();
    Q_INVOKABLE bool isStarted();
    Q_INVOKABLE bool capture();
    ImageProvider *m_pImageProvider;
    void setTwainPath(QString path);
signals:
    void imageOutput();
    void sendCameraList(QVariantList data);
public slots:
    void exit();
    void getCameraList();
    void grabImage(QImage image);
    void selectCamera(QString camera);
private:
    QCamera             *m_camera;
    QtCameraCapture     *m_cameraCapture;
    QCameraInfo         m_cameraDeviceInfo;
    bool                m_started;
    QList<QCameraInfo>  cameras;
    std::vector<QString>cameraNames;
    TwGlue              m_glue;
    QString             oldTwainPath;
};
