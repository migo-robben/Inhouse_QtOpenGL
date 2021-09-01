#ifndef QTREFERENCE_MYTIMER_H
#define QTREFERENCE_MYTIMER_H

#include <boost/chrono.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#include <QString>

using namespace boost::chrono;

class myTimer {
public:
    explicit myTimer() = default;

    void setStartPoint() { startPoint = system_clock::now(); }
    void setEndPoint() { endPoint = system_clock::now(); }
    double getDuration() {
        auto duration = duration_cast<boost::chrono::microseconds>(endPoint - startPoint);
        return double(duration.count()) * microseconds::period::num / microseconds::period::den * 1000.0;
    }

    void printDuration(const QString& prefix) {
        spdlog::info("\n\t{} consume time: {} ms", prefix.toStdString(), getDuration());
    }

private:
    time_point<system_clock> startPoint;
    time_point<system_clock> endPoint;
};

#endif //QTREFERENCE_MYTIMER_H
