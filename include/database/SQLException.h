//
// Created by Matthew.Sirman on 21/08/2020.
//

#ifndef CONTRACTS_INTERNAL_SQLEXCEPTION_H
#define CONTRACTS_INTERNAL_SQLEXCEPTION_H

#include <exception>
#include <string>
#include <sstream>

namespace sql {

    // SQLException
    // Represents an exception in the SQL system
    class SQLException : public std::exception {
    public:
        // Constructor with an arbitrary message
        explicit SQLException(const std::string &message);

        // Constructor with a specific state and error message from the SQL internal error scheme
        SQLException(const std::string &sqlState, const std::string &sqlError);

        // Getter for the internal state message (independently)
        const std::string state() const;

        // Getter for the internal error message (independently)
        const std::string error() const;

    private:
        // Internal state and error messages
        std::string sqlState, sqlError;
    };

    // UnknownSQLException
    // Represents an unknown exception in the SQL system, i.e. one which did not return
    // a recognised error code
    class UnknownSQLException : public std::exception {
    public:
        UnknownSQLException();
    };

}

#endif //CONTRACTS_INTERNAL_SQLEXCEPTION_H
