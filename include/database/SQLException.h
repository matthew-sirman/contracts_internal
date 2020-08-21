//
// Created by Matthew.Sirman on 21/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_SQLEXCEPTION_H
#define CONTRACTS_SITE_CLIENT_SQLEXCEPTION_H

#include <exception>
#include <string>
#include <sstream>

class SQLException : public std::exception {
public:
    SQLException(const std::string &message);

    SQLException(const std::string &sqlState, const std::string &sqlError);

    const std::string state() const;

    const std::string error() const;

private:
    std::string sqlState, sqlError;
};

class UnknownSQLException : public std::exception {
public:
    UnknownSQLException();
};

#endif //CONTRACTS_SITE_CLIENT_SQLEXCEPTION_H
