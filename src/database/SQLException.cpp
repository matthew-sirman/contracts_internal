//
// Created by Matthew.Sirman on 21/08/2020.
//

#include "../../include/database/SQLException.h"

using namespace sql;

SQLException::SQLException(const std::string &message)
        : std::exception(message.c_str()) {

}

// Passes the state and error messages in a formatted string to the base exception
SQLException::SQLException(const std::string &sqlState, const std::string &sqlError)
        : std::exception(("SQL Error (" + sqlState + "): " + sqlError).c_str()),
          sqlState(sqlState), sqlError(sqlError) {

}

const std::string SQLException::state() const {
    return sqlState;
}

const std::string SQLException::error() const {
    return sqlError;
}

UnknownSQLException::UnknownSQLException()
        : std::exception("Unknown SQL Exception") {

}
