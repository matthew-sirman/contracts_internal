//
// Created by Matthew.Sirman on 20/08/2020.
//

#include "../../include/database/SQLSession.h"

using namespace sql;

SQLSession::SQLSession() {
    // Create the environment handle
    sqlEnvHandle.allocate();
    // Set the ODBC version on this handle
    handleInternalError(SQLSetEnvAttr(sqlEnvHandle.get(), SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0),
                        sqlEnvHandle);
}

SQLSession::~SQLSession() {
    // Close the connection to the database
    closeConnection();
}

void SQLSession::connect(const std::string &dsn, const std::string &userID, const std::string &password) {
    // Create the connection handle
    sqlConnHandle.allocate(sqlEnvHandle);

    // Declare the connection string
    std::stringstream connectionString;

    // Add the appropriate fields to the connection string
    connectionString << "DSN=" << dsn << ";";
    connectionString << "UID=" << userID << ";";
    connectionString << "PWD=" << password << ";";

    // Attempt to connect to the database. This will return a code indicating success for failure
    switch (SQLDriverConnect(sqlConnHandle.get(), nullptr, (SQLCHAR *) connectionString.str().c_str(),
                             SQL_NTS, nullptr, SQL_NTS, nullptr, SQL_DRIVER_NOPROMPT)) {
        case SQL_SUCCESS:
        case SQL_SUCCESS_WITH_INFO:
            // Flag that we are connected
            connected = true;
            break;
        case SQL_INVALID_HANDLE:
        case SQL_ERROR:
            // Flag that we aren't connected then throw an exception
            connected = false;
            throw sqlConnHandle.getError();
        default:
            // Throw an unknown exception - this code wasn't picked up from the return value
            throw UnknownSQLException();
    }
}

void SQLSession::execute(const std::string &sql) {
    // Set up the internal statement handle ready for use if necessary
    setupStatementHandle();

    // Execute the SQL query passed in
    handleInternalError(SQLExecDirect(sqlStatementHandle.get(), (SQLCHAR *) sql.c_str(), SQL_NTS), sqlStatementHandle);
}

sql::QueryResult SQLSession::executeQuery(const std::string &sql) {
    // Create a handle for this query - this allows for "query parallelism" i.e. having multiple queries which do
    // not necessarily require to be executed in order.
    SQLStatementHandle queryStatementHandle;
    queryStatementHandle.allocate(sqlConnHandle);

    // Execute the query with
    handleInternalError(SQLExecDirect(queryStatementHandle.get(), (SQLCHAR *) sql.c_str(), SQL_NTS), queryStatementHandle);

    // Return a a query result
    return QueryResult(std::move(queryStatementHandle));
}

sql::Table SQLSession::table(const std::string &tableName) {
    std::unique_ptr<sql::internal::QueryBuilder> builder = std::make_unique<sql::internal::QueryBuilder>(this);
    // Return a table object based on the passed in table name
    return Table(tableName, std::nullopt, std::move(builder));
}

sql::Table SQLSession::table(const std::string &tableName, const std::string &tableAlias) {
    std::unique_ptr<sql::internal::QueryBuilder> builder = std::make_unique<sql::internal::QueryBuilder>(this);
    return Table(tableName, tableAlias, std::move(builder));
}

void SQLSession::closeConnection() {
    // If the object is connected to the database
    if (connected) {
        // Disconnect
        SQLDisconnect(sqlConnHandle.get());
        // Flag that we are now disconnected
        connected = false;
    }
}

void SQLSession::setupStatementHandle() {
    // If we already have a statement handle, there is nothing to do
    if (sqlStatementHandle) {
        return;
    }
    // Otherwise, we create a new handle
    sqlStatementHandle.allocate(sqlConnHandle);
}

template<HandleType handleType>
void SQLSession::handleInternalError(SQLRETURN code, const SQLSafeHandle<handleType> &handle) const {
    // Switch the error code
    switch (code) {
        case SQL_SUCCESS:
        case SQL_SUCCESS_WITH_INFO:
            // If the call returned successful, we have nothing to do so just break.
            break;
        case SQL_ERROR:
            // If the call returned an error, we throw an exception based upon the SQL error
            throw handle.getError();
        case SQL_INVALID_HANDLE:
            // If the call returned that the handle is invalid, we throw a custom exception
            throw SQLException("Call to SQL function was made with an invalid handle.");
        default:
            // If the call returned any other code then we throw an unknown exception
            throw UnknownSQLException();
    }
}

