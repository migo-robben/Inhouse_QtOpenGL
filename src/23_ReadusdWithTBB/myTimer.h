#ifndef QTREFERENCE_MYTIMER_H
#define QTREFERENCE_MYTIMER_H

#include <stack>

#include <boost/chrono.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#include <QString>

using namespace boost::chrono;


struct PointInfo {
    std::string _prefix;
    time_point<system_clock> _time_point;
    PointInfo(std::string prefix, time_point<system_clock> time_point) : _prefix(prefix), _time_point(time_point) {}
};


class myTimer {
public:
    explicit myTimer() = default;

    double getDuration(time_point<system_clock> start_point, time_point<system_clock> end_point) {
        auto duration = duration_cast<boost::chrono::microseconds>(end_point - start_point);
        return double(duration.count()) * microseconds::period::num / microseconds::period::den * 1000.0;
    }

    void setStartPoint(std::string prefix) {
        time_point<system_clock> start_point = system_clock::now();
        PointInfo point_info(prefix, start_point);
        _point_stack.push(point_info);
        spdlog::info("\tStart {}", prefix);
    }

    void setEndPoint() {
        setEndPoint("", true);
    }

    void setEndPoint(std::string prefix) {
        setEndPoint(prefix, true);
    }

    void setEndPoint(bool is_print_str) {
        setEndPoint("", is_print_str);
    }

    void setEndPoint(std::string prefix, bool is_print_str) {
        time_point<system_clock> end_point = system_clock::now();

        if (_point_stack.empty()) {
            if (is_print_str) {
                spdlog::info("\tEnd {}", prefix);
            }
        }
        else {
            PointInfo point_info = _point_stack.top();
            _point_stack.pop();

            if (prefix.size() == 0) {
                prefix = point_info._prefix;
            }

            if (is_print_str) {
                point_info._prefix;
                spdlog::info("\tEnd {} time: {} ms", prefix, getDuration(point_info._time_point, end_point));
            }
        }
    }

private:
    std::stack<PointInfo> _point_stack;

};

#endif //QTREFERENCE_MYTIMER_H
