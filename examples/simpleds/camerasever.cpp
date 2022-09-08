#include "camerasever.h"
//-----------------------------------------------------------------------------------------
QtCameraCapture::QtCameraCapture(QObject *parent) : QAbstractVideoSurface(parent)
{

}

QList<QVideoFrame::PixelFormat> QtCameraCapture::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    Q_UNUSED(handleType);
    return QList<QVideoFrame::PixelFormat>()
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::Format_ARGB32_Premultiplied
            << QVideoFrame::Format_RGB32
            << QVideoFrame::Format_AdobeDng;
}

bool QtCameraCapture::present(const QVideoFrame &frame)
{
    if (frame.isValid()) {
        QVideoFrame cloneFrame(frame);
        cloneFrame.map(QAbstractVideoBuffer::ReadOnly);
        int format= QVideoFrame::imageFormatFromPixelFormat(cloneFrame.pixelFormat());
        if (format != QImage::Format_Invalid)
        {
            const QImage image(cloneFrame.bits(),
                               cloneFrame.width(),
                               cloneFrame.height(),
                               QVideoFrame::imageFormatFromPixelFormat(cloneFrame.pixelFormat()));
            emit frameAvailable(image);
            cloneFrame.unmap();
            return true;
        }
        else {
            if (cloneFrame.pixelFormat() == QVideoFrame::Format_YUYV)
            {
                qDebug() << "QVideoFrame::Format_YUYV";
            }
            else
            {
                int nbytes = frame.mappedBytes();
                emit frameAvailable(QImage::fromData(frame.bits(), nbytes));
            }
        }
        cloneFrame.unmap();
        return true;
    }
    else {
        qDebug() << "frame is not valid";
    }
    return false;
}
//--------------------------------------------------------------------------------------------
QtCamera::QtCamera(QCameraInfo cameraInfo, QObject *parent, const TwGlue& glue) :
    QObject(parent),
    m_glue(glue)
{
    m_started = false;
    m_camera = NULL;
    m_cameraCapture = NULL;

    cameras = QCameraInfo::availableCameras();
    foreach (cameraInfo, cameras) {
        cameraNames.push_back(cameraInfo.description());
    }

    m_cameraCapture = new QtCameraCapture;
    connect(m_cameraCapture, SIGNAL(frameAvailable(QImage)), this, SLOT(grabImage(QImage)));
    m_pImageProvider = new ImageProvider;
}

QtCamera::~QtCamera()
{
    if(isStarted()) stop();
    if (m_camera != NULL) {
        delete m_camera;
        m_camera = NULL;
    }
    if (m_cameraCapture != NULL) {
        delete m_cameraCapture;
        m_cameraCapture = NULL;
    }
    m_glue.m_cancel(oldTwainPath);
}

void QtCamera::setTwainPath(QString path){
    oldTwainPath = path;
}

void QtCamera::getCameraList(){
    QVariantList varList;
    for(int i=0;i<cameraNames.size();i++){
        varList.push_back(cameraNames[i]);
    }
    sendCameraList(varList);
}

void QtCamera::selectCamera(QString camera){
    qDebug()<<"selectCamer:  " + camera;
    for(int i=0;i<cameras.size();i++){
        if(camera == cameras[i].description()){
            if(isStarted()) stop();
            m_cameraDeviceInfo = cameras[i];
            start();
        }
    }
}

bool QtCamera::start()
{
    if (m_started) return false;
    else {
        if (m_cameraDeviceInfo.isNull()) return false;
        else {
            m_camera = new QCamera(m_cameraDeviceInfo);
            m_camera->setViewfinder(m_cameraCapture);
            m_camera->start();
            m_started = true;
        }
    }
    return true;
}

bool QtCamera::stop()
{
    m_camera->stop();
    m_started = false;
    return true;
}

bool QtCamera::isStarted()
{
    return m_started;
}

bool QtCamera::capture()
{
    if (! m_started) {
        return false;
    }
    m_glue.m_scan(m_pImageProvider->img);
    return true;
    //return m_image.save(filePath, "jpg");
}

void QtCamera::grabImage(QImage image)
{
    QMatrix matrix;
    matrix.rotate(180);
    QImage imgRatate = image.transformed(matrix);
    //m_image = image.mirrored(false, true);
    m_pImageProvider->img = imgRatate;
    emit imageOutput();
}
