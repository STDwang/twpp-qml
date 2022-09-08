#include <QQuickImageProvider>
#pragma once
class ImageProvider : public QQuickImageProvider
{
public:
	ImageProvider();
	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize);
	QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize);

    QImage img;
};
