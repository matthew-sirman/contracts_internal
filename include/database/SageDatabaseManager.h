//
// Created by Matthew.Sirman on 20/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_SAGEDATABASEMANAGER_H
#define CONTRACTS_SITE_CLIENT_SAGEDATABASEMANAGER_H

#include <sstream>

#include "queryConstructions.h"
#include "SQLException.h"

class SageDatabaseManager {
public:
    // Constructor
    SageDatabaseManager();

    // Destructor
    ~SageDatabaseManager();

    // Connect to the ODBC database through a predefined DSN
    void connect(const std::string &dsn, const std::string &userID, const std::string &password);

    // Execute an SQL statement which does not return any data
    void execute(const std::string &sql);

    // Execute an SQL query
    sql::QueryResult executeQuery(const std::string &sql);

    sql::Table table(const std::string &tableName);

    // Terminate the connection
    void closeConnection();

private:
    enum HandleType {
        STATEMENT_HANDLE,
        ENVIRONMENT_HANDLE,
        CONNECTION_HANDLE
    };

    // Handles for the internal connection and environment
    SQLHANDLE sqlConnHandle = nullptr, sqlEnvHandle = nullptr, sqlStatementHandle = nullptr;

    // Flag indicating whether the manager is currently connected to the database
    bool connected = false;

    // Method to setup statement handle creation
    void setupStatementHandle();

    // Handles an internal error by throwing an exception where necessary
    void handleInternalError(SQLRETURN code, HandleType handleType) const;

    // Gets an error message from the underlying SQL handles
    SQLException getError(HandleType handleType) const;
};

#endif //CONTRACTS_SITE_CLIENT_SAGEDATABASEMANAGER_H
