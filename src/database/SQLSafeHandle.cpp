//
// Created by Matthew.Sirman on 24/08/2020.
//

#include "../../include/database/SQLSafeHandle.h"

using namespace sql;

template<HandleType handleType>
SQLSafeHandle<handleType>::SQLSafeHandle()
        : __handle(nullptr) {

}

template<HandleType handleType>
SQLSafeHandle<handleType>::SQLSafeHandle(SQLHANDLE handle)
        : __handle(handle) {

}

template<HandleType handleType>
SQLSafeHandle<handleType>::SQLSafeHandle(SQLSafeHandle<handleType> &&other) noexcept
        : __handle(other.__handle) {
    // Invalidate the original handle as we are acquiring ownership
    other.invalidate();
}

template<HandleType handleType>
SQLSafeHandle<handleType>::~SQLSafeHandle() {
    // If the handle is valid, destroy it
    if (__handle) {
        destroy();
    }
}

template<HandleType handleType>
SQLSafeHandle<handleType> &SQLSafeHandle<handleType>::operator=(SQLNullHandle) noexcept {
    // Destroy the handle - we are explicitly nullifying this handle
    destroy();
    return *this;
}

template<HandleType handleType>
SQLSafeHandle<handleType> &SQLSafeHandle<handleType>::operator=(SQLSafeHandle<handleType> &&other) noexcept {
    // If this is self assignment, do nothing
    if (this == &other) {
        return *this;
    }

    // Otherwise move the internal handle from the other handle object to this and invalidate
    // the other handle object as we are acquiring the handle
    this->__handle = other.__handle;
    other.invalidate();

    return *this;
}

template<HandleType handleType>
constexpr SQLSafeHandle<handleType>::operator bool() noexcept {
    return __handle;
}

template<HandleType handleType>
constexpr bool SQLSafeHandle<handleType>::operator==(const SQLSafeHandle<handleType> &other) noexcept {
    return __handle == other.__handle;
}

template<HandleType handleType>
constexpr bool SQLSafeHandle<handleType>::operator!=(const SQLSafeHandle<handleType> &other) noexcept {
    return __handle != other.__handle;
}

template<HandleType handleType>
void SQLSafeHandle<handleType>::allocate() {
    // Allocate the handle and switch the return value
    switch (SQLAllocHandle(handleType, SQL_NULL_HANDLE, &__handle)) {
        case SQL_SUCCESS:
        case SQL_SUCCESS_WITH_INFO:
            // If successful, simply return
            return;
        case SQL_ERROR:
            // Throw a default exception - there is no input handle to get an error from
            throw SQLException("Failed to allocate handle (no input handle to catch error from).");
        default:
            // If the return value was an unrecognised code, throw an unknown exception
            throw UnknownSQLException();
    }
}

template<HandleType handleType>
SQLHANDLE SQLSafeHandle<handleType>::get() const {
    return __handle;
}

template<HandleType handleType>
void SQLSafeHandle<handleType>::invalidate() {
    // Set the internal handle to the null handle
    __handle = SQL_NULL_HANDLE;
}

template<HandleType handleType>
void SQLSafeHandle<handleType>::destroy() {
    // Free the internal handle with the handle type specified in the template
    SQLFreeHandle(handleType, __handle);
    invalidate();
}

template<HandleType handleType>
SQLException SQLSafeHandle<handleType>::getError() const {
    // Declare buffers for the state and error message returned
    SQLCHAR sqlState[256], errorMessage[256];
    // Declare the native error parameter (unused)
    SQLINTEGER nativeError = 0;
    // Declare a variable to store the returned error message length
    SQLSMALLINT errorMessageLength;

    // We call the get diagnostic function to get the state and error message
    SQLGetDiagRec(handleType, __handle, 1, sqlState, &nativeError, errorMessage, sizeof(errorMessage),
                  &errorMessageLength);

    // Return an SQLException object with the state and error message
    return SQLException(std::string((const char *) sqlState),
                        std::string((const char *) errorMessage, errorMessageLength));
}

// Declare explicitly each of the handle types so the appropriate template methods are instantiated correctly without
// needing to be in the header
template
struct sql::SQLSafeHandle<STATEMENT_HANDLE>;
template
struct sql::SQLSafeHandle<ENVIRONMENT_HANDLE>;
template
struct sql::SQLSafeHandle<CONNECTION_HANDLE>;
