//
// Created by Matthew.Sirman on 24/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_SQLSAFEHANDLE_H
#define CONTRACTS_SITE_CLIENT_SQLSAFEHANDLE_H

#include <Windows.h>
#include <sqlext.h>
#include <sql.h>
#include <sqltypes.h>

#include "SQLException.h"

namespace sql {

    // SQLNullHandle
    // Represents a tag to disengage safe handles
    struct SQLNullHandle {
        // Used to construct a null handle
        enum class __Construct {
            __Token
        };

        // Declared as constexpr to make the type a literal
        explicit constexpr SQLNullHandle(__Construct) { }
    };

    // Tag to disengage safe handles
    inline constexpr SQLNullHandle nullHandle { SQLNullHandle::__Construct::__Token };

    enum HandleType {
        ENVIRONMENT_HANDLE = SQL_HANDLE_ENV,
        CONNECTION_HANDLE = SQL_HANDLE_DBC,
        STATEMENT_HANDLE = SQL_HANDLE_STMT
    };

    // SQLSafeHandle
    // Wrapper type for handling an SQLHANDLE in a C++ style object
    // Uses move semantics to pass around the handle without destroying the internal handle
    // until it goes out of scope
    template<HandleType handleType>
    struct SQLSafeHandle {
        // Default constructor
        SQLSafeHandle();

        // Constructor specifying the internal handle
        SQLSafeHandle(SQLHANDLE handle);

        // Deleted copy constructor
        SQLSafeHandle(const SQLSafeHandle<handleType> &) = delete;

        // Move constructor
        SQLSafeHandle(SQLSafeHandle<handleType> &&other) noexcept;

        // Destructor
        ~SQLSafeHandle();

        // Deleted assignment operator
        SQLSafeHandle<handleType> &operator=(const SQLSafeHandle<handleType> &) = delete;

        // Assignment to null handle operator
        SQLSafeHandle<handleType> &operator=(SQLNullHandle) noexcept;

        // Move assignment operator
        SQLSafeHandle<handleType> &operator=(SQLSafeHandle<handleType> &&other) noexcept;

        // Bool operator. Returns true if the handle is valid otherwise false
        constexpr operator bool() noexcept;

        // Equality operator between two safe handles
        constexpr bool operator==(const SQLSafeHandle<handleType> &other) noexcept;

        // Inequality operator between two safe handles
        constexpr bool operator!=(const SQLSafeHandle<handleType> &other) noexcept;

        // Allocates a new handle with no input
        void allocate();

        // Allocates a handle based off an input handle
        template<HandleType inputHandleType>
        void allocate(const SQLSafeHandle<inputHandleType> &inputHandle);

        // Getter for internal handle
        SQLHANDLE get() const;

        // Get the appropriate SQL exception from the internal handle
        SQLException getError() const;

    private:
        // Invalidate this handle
        void invalidate();

        // Destroy the actual handle itself
        void destroy();

        // The internal handle pointer
        SQLHANDLE __handle;
    };

    // Comparison operators with null handle types

    template<HandleType lhsType>
    constexpr bool operator==(const SQLSafeHandle<lhsType> &lhs, SQLNullHandle) noexcept {
        return !lhs;
    }

    template<HandleType lhsType>
    constexpr bool operator!=(const SQLSafeHandle<lhsType> &lhs, SQLNullHandle) noexcept {
        return static_cast<bool>(lhs);
    }

    template<HandleType rhsType>
    constexpr bool operator==(SQLNullHandle, const SQLSafeHandle<rhsType> &rhs) noexcept {
        return !rhs;
    }

    template<HandleType rhsType>
    constexpr bool operator!=(SQLNullHandle, const SQLSafeHandle<rhsType> &rhs) noexcept {
        return static_cast<bool>(rhs);
    }

    constexpr bool operator==(SQLNullHandle, SQLNullHandle) noexcept {
        return true;
    }

    constexpr bool operator!=(SQLNullHandle, SQLNullHandle) noexcept {
        return false;
    }

    template<HandleType handleType>
    template<HandleType inputHandleType>
    void SQLSafeHandle<handleType>::allocate(const SQLSafeHandle<inputHandleType> &inputHandle) {
        // Allocate the handle and switch the return value
        switch (SQLAllocHandle(handleType, inputHandle.get(), &__handle)) {
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
                // If successful, simply return
                return;
            case SQL_ERROR:
                // If there was an error, throw the getError function on the input handle.
                throw inputHandle.getError();
            default:
                // If the return value was an unrecognised code, throw an unknown exception
                throw UnknownSQLException();
        }
    }

    // Define aliases for the different handle types
    typedef SQLSafeHandle<ENVIRONMENT_HANDLE> SQLEnvironmentHandle;
    typedef SQLSafeHandle<CONNECTION_HANDLE> SQLConnectionHandle;
    typedef SQLSafeHandle<STATEMENT_HANDLE> SQLStatementHandle;

}

#endif //CONTRACTS_SITE_CLIENT_SQLSAFEHANDLE_H
