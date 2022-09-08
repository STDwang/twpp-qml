#ifndef TWGLUE_HPP
#define TWGLUE_HPP

#include <functional>
#include <QImage>
struct TwGlue {

    TwGlue(const std::function<void(QImage)>& scan, const std::function<void(QString)>& cancel) :
        m_scan(scan), m_cancel(cancel){}

    std::function<void(QImage)> m_scan;
    std::function<void(QString)> m_cancel;
};

#endif // TWGLUE_HPP
