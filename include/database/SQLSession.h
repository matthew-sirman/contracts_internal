//
// Created by Matthew.Sirman on 20/08/2020.
//

#ifndef CONTRACTS_INTERNAL_SAGEDATABASEMANAGER_H
#define CONTRACTS_INTERNAL_SAGEDATABASEMANAGER_H

#include <sstream>

#include "queryConstructions.h"
#include "SQLException.h"

namespace sql {

    // SQLSession
    // SQL Session representing a queryable connection to the ODBC.
    // Essentially wraps the low level C ODBC interface with a C++ style interface
    class SQLSession {
    public:
        // Constructor
        SQLSession();

        // Destructor
        ~SQLSession();

        // Connect to the ODBC database through a predefined DSN
        void connect(const std::string &dsn, const std::string &userID, const std::string &password);

        // Execute an SQL statement which does not return any data
        void execute(const std::string &sql);

        // Execute an SQL query
        sql::QueryResult executeQuery(const std::string &sql);

        // Gets a queryable table object for the C++ style query builder interface
        sql::Table table(const std::string &tableName);

        // Gets a queryable table object with an alias for the C++ style query builder interface
        sql::Table table(const std::string &tableName, const std::string &tableAlias);

        // Terminate the connection
        void closeConnection();

    private:
        // Handles for the internal connection and environment
        SQLConnectionHandle sqlConnHandle;
        SQLEnvironmentHandle sqlEnvHandle;
        SQLStatementHandle sqlStatementHandle;

        // Flag indicating whether the manager is currently connected to the database
        bool connected = false;

        // Method to setup statement handle creation
        void setupStatementHandle();

        // Handles an internal error by throwing an exception where necessary
        template<HandleType handleType>
        void handleInternalError(SQLRETURN code, const SQLSafeHandle<handleType> &handle) const;
    };

}

#endif //CONTRACTS_INTERNAL_SAGEDATABASEMANAGER_H
