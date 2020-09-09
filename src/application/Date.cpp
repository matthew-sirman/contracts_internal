//
// Created by Matthew.Sirman on 09/09/2020.
//

#include <ctime>

#include "../../include/application/Date.h"

Date::Date()
        : year(1970), month(01), day(01) {

}

Date::Date(unsigned int year, unsigned int month, unsigned int day)
        : year(year), month(month), day(day) {

}

Date Date::today() {
    time_t now = time(nullptr);
    std::tm *timePoint = std::localtime(&now);
    return { (unsigned) (timePoint->tm_year + 1900), (unsigned) (timePoint->tm_mon + 1), (unsigned) timePoint->tm_mday };
}
