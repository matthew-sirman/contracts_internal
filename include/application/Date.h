//
// Created by Matthew.Sirman on 09/09/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_DATE_H
#define CONTRACTS_SITE_CLIENT_DATE_H

struct Date {
public:
    Date();

    Date(unsigned year, unsigned month, unsigned day);

    static Date today();

private:
    unsigned short year;
    unsigned char month, day;

};


#endif //CONTRACTS_SITE_CLIENT_DATE_H
