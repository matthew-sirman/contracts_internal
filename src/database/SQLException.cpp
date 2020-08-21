//
// Created by Matthew.Sirman on 21/08/2020.
//

#include "../../include/database/SQLException.h"

SQLException::SQLException(const std::string &message)
        : std::exception(message.c_str()) {

}

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

UnknownSQLException::UnknownSQLException() : std::exception("Unknown SQL Exception") {

}
