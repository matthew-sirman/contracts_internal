//
// Created by Matthew.Sirman on 20/08/2020.
//

#include "../../include/database/SQLSession.h"

using namespace sql;

SQLSession::SQLSession() {
    // Create the environment handle
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle) != SQL_SUCCESS) {
        // If there is an error, throw an exception
        throw SQLException("Failed to allocated environment handle.");
    }
    // Set the ODBC version on this handle
    handleInternalError(SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0),
                        ENVIRONMENT_HANDLE);
}

SQLSession::~SQLSession() {
    // Close the connection to the database
    closeConnection();
    // Free the environment and database connection handles
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
}

void SQLSession::connect(const std::string &dsn, const std::string &userID, const std::string &password) {
    // Create the connection handle
    handleInternalError(SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle), ENVIRONMENT_HANDLE);

    // Declare the connection string
    std::stringstream connectionString;

    // Add the appropriate fields to the connection string
    connectionString << "DSN=" << dsn << ";";
    connectionString << "UID=" << userID << ";";
    connectionString << "PWD=" << password << ";";

    // Attempt to connect to the database. This will return a code indicating success for failure
    switch (SQLDriverConnect(sqlConnHandle, nullptr, (SQLCHAR *) connectionString.str().c_str(),
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
            throw getError(CONNECTION_HANDLE);
        default:
            // Throw an unknown exception - this code wasn't picked up from the return value
            throw UnknownSQLException();
    }
}

void SQLSession::execute(const std::string &sql) {
    // Set up the statement handle ready for use if necessary
    setupStatementHandle();

    // Execute the SQL query passed in
    handleInternalError(SQLExecDirect(sqlStatementHandle, (SQLCHAR *) sql.c_str(), SQL_NTS), STATEMENT_HANDLE);
}

sql::QueryResult SQLSession::executeQuery(const std::string &sql) {
    // Execute the query
    execute(sql);

    // Construct a QueryResult object so the caller may access the returned data
    return QueryResult(sqlStatementHandle);
}

sql::Table SQLSession::table(const std::string &tableName) {
    // Return a table object based on the passed in table name
    return Table(tableName, sql::internal::QueryBuilder(this));
}

void SQLSession::closeConnection() {
    // If the object is connected to the database
    if (connected) {
        // Disconnect
        SQLDisconnect(sqlConnHandle);
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
    handleInternalError(SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStatementHandle), CONNECTION_HANDLE);
}

void SQLSession::handleInternalError(SQLRETURN code, HandleType handleType) const {
    // Switch the error code
    switch (code) {
        case SQL_SUCCESS:
        case SQL_SUCCESS_WITH_INFO:
            // If the call returned successful, we have nothing to do so just break.
            break;
        case SQL_ERROR:
            // If the call returned an error, we throw an exception based upon the SQL error
            throw getError(handleType);
        case SQL_INVALID_HANDLE:
            // If the call returned that the handle is invalid, we throw a custom exception
            throw SQLException("Call to SQL function was made with an invalid handle.");
        default:
            // If the call returned any other code then we throw an unknown exception
            throw UnknownSQLException();
    }
}

SQLException SQLSession::getError(SQLSession::HandleType handleType) const {
    // Declare buffers for the state and error message returned
    SQLCHAR sqlState[256], errorMessage[256];
    // Declare the native error parameter (unused)
    SQLINTEGER nativeError = 0;
    // Declare a variable to store the returned error message length
    SQLSMALLINT errorMessageLength;

    // The handle information to call the diagnostic function with
    SQLSMALLINT handleCode;
    SQLHANDLE handle;

    // We set the handle code and handle pointer depending on what sort of error we are looking for
    switch (handleType) {
        case STATEMENT_HANDLE:
            handleCode = SQL_HANDLE_STMT;
            handle = sqlStatementHandle;
            break;
        case ENVIRONMENT_HANDLE:
            handleCode = SQL_HANDLE_ENV;
            handle = sqlEnvHandle;
            break;
        case CONNECTION_HANDLE:
            handleCode = SQL_HANDLE_DBC;
            handle = sqlConnHandle;
            break;
    }

    // We call the get diagnostic function to get the state and error message
    SQLGetDiagRec(handleCode, handle, 1, sqlState, &nativeError, errorMessage, sizeof(errorMessage),
                  &errorMessageLength);

    // Return an SQLException object with the state and error message
    return SQLException(std::string((const char *) sqlState),
                        std::string((const char *) errorMessage, errorMessageLength));
}

