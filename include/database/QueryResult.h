//
// Created by Matthew.Sirman on 20/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_QUERYRESULT_H
#define CONTRACTS_SITE_CLIENT_QUERYRESULT_H

//#include <Windows.h>
//#include <sqlext.h>
//#include <sql.h>
//#include <sqltypes.h>

#include <string>
#include <memory>

#include "SQLSafeHandle.h"

#define MAX_QUERY_STRING_LENGTH 1024

namespace sql {

    // Forward declarations
    class SQLSession;

    class QueryResult;

    class ColumnSet;

    class Row;

    template<typename T>
    struct Column;

    // Iterator object for range based for loops
    struct QueryResultRowIterator {
        // Friend the overarching query result class
        friend class QueryResult;

    public:
        // Dereference operator - returns a row at each point representing
        // the current row in the range
        Row operator*() const;

        // Pre increment operator
        QueryResultRowIterator &operator++();

        // Post increment operator
        QueryResultRowIterator operator++(int);

        // Equality operator
        bool operator==(const QueryResultRowIterator &other) const;

        // Inequality operator
        bool operator!=(const QueryResultRowIterator &other) const;

    private:
        // Private hidden constructor callable from the QueryResult object when setting up the iterator range
        QueryResultRowIterator(size_t iterPosition, QueryResult &resultObject);

        // The current position of this iterator. Goes from 0 (first element) to the number of returned records
        size_t iterPosition;

        // The calling result object to return the row from
        QueryResult &resultObject;
    };

    // ColumnSetItemProxy
    // Represents a proxy for a column in a ColumnSet used to have a non templated subscript operator
    struct ColumnSetItemProxy {
        // Friend the column set class
        friend class ColumnSet;

        // Cast operator to a type T (which should always be a Column of the appropriate data type)
        template<typename T>
        inline operator Column<T>() const {
            // Returns a column object with a reference to this object so it can use the getItem method
            return Column<T>(*this);
        }

        // Wrapper for the query result's internal get method to get the value for the current row at the
        // index of this column
        template<typename T>
        inline T getItem() const {
            return resultObject.get<T>(columnIndex);
        }

    private:
        // Private constructor callable by the ColumnSet class
        inline ColumnSetItemProxy(const QueryResult &resultObject, size_t index) : resultObject(resultObject),
                                                                                   columnIndex(index) {}

        // Reference to the query result object for the get method
        const QueryResult &resultObject;
        // The index of the column this proxy is associated with
        size_t columnIndex;
    };

    // Column
    // Represents a Column of type T in the database. This column will represent the data at the given index for
    // the current row in the result object
    template<typename T>
    struct Column {
        // Constructor for the column based upon the corresponding column proxy
        inline Column(ColumnSetItemProxy item) : item(item) {}

        // Cast operator to the column's type
        inline operator T() const {
            return get();
        }

        // Explicit get method to return the data in this column at the current row
        inline T get() const {
            return item.getItem<T>();
        };

    private:
        // Private proxy object to actually retrieve the data from the query result object itself
        ColumnSetItemProxy item;
    };

    // ColumnSet
    // Defines an a set of columns which can be indexed to return a column proxy which can be converted (implicitly) to
    // a Column object
    class ColumnSet {
        // Friend the QueryResult so it can call the private constructor
        friend class QueryResult;

    public:
        // Subscript operator to return the column at the specified index
        inline ColumnSetItemProxy operator[](size_t index) const {
            // Return a proxy for this column
            return ColumnSetItemProxy(resultObject, index);
        }

    private:
        // Private constructor which specifies the query result object
        inline ColumnSet(const QueryResult &resultObject) : resultObject(resultObject) {}

        // A reference to the query result object for creating the column objects
        const QueryResult &resultObject;
    };

    struct RowItemProxy {
        // Friend the Row class so it can call the private constructor
        friend class Row;

        // Template cast method. Calls the private get method in the QueryResult object assigned
        template<typename T>
        inline operator T() const {
            return resultObject.get<T>(index);
        }

        template<typename T>
        inline T get() const {
            return resultObject.get<T>(index);
        }

    private:
        // Constructor taking a QueryResult object and the index of the column of interest
        inline RowItemProxy(const QueryResult &queryResult, size_t i)
                : resultObject(queryResult), index(i) {

        }

        // Private QueryResult object this proxy is associated with
        const QueryResult &resultObject;
        // The index of the column of the query result this proxy is associated with
        size_t index;
    };

    // Row
    // Represents the current row in the query
    class Row {
        // Friend the QueryResult class so it can call the private constructor
        friend class QueryResult;

    public:
        // Subscript operator to return a proxy for the item at the given index
        inline RowItemProxy operator[](size_t index) const {
            // Return a proxy for this index
            return RowItemProxy(resultObject, index);
        }

    private:
        // Private constructor which specifies the result object
        inline Row(const QueryResult &resultObject) : resultObject(resultObject) {}

        // The result object this row is associated with
        const QueryResult &resultObject;
    };

    // QueryResult class
    // Represents the results from a query made in the SQL.
    class QueryResult {
        // Friend the session class so it can call the private constructor
        friend class SQLSession;

        // Friend the row item proxy so it can call the private get method
        friend struct RowItemProxy;

        // Friend the column set item proxy so it can call the private get method
        friend struct ColumnSetItemProxy;

    public:
        /*// Copy constructor
        QueryResult(const QueryResult &queryResult);

        ~QueryResult();*/

        // Fetches the next row from the dataset. The getter methods for row items will retrieve
        // items from the next row once called
        void fetchNextRow();

        // Gets the first N items from the current row, where N is the number of values specified. Each is written to
        // the corresponding reference variable passed in.
        template<typename ...Values>
        void getRow(Values &...values) const;

        // Gets the number of rows returned by the query
        size_t rowCount() const;

        // Gets the current row index
        size_t currentRow() const;

        // Begin iterator for range based for loops
        QueryResultRowIterator begin();

        // End iterator for range based for loops
        QueryResultRowIterator end();

        // Gets the current row. This can be indexed to retrieve an individual value
        const Row row;

        // Gets the set of columns. This can be indexed to retrieve an individual value
        const ColumnSet columns;

    protected:
        // Protected constructor. This is hidden so only the session can create a QueryResult
        QueryResult(SQLSafeHandle<STATEMENT_HANDLE> &&sqlStatementHandle);

    private:
        // A handle for the statement which contains the internal row results
        SQLSafeHandle<STATEMENT_HANDLE> sqlStatementHandle;

        // Current row index in the query - incremented each time fetchNextRow is called
        size_t currentRowIndex = 0;

        // Recursive case for the implementation of the getRow method
        template<typename T, typename ...Values>
        void impl_getRow(size_t index, T &t, Values &...values) const;

        // Base case for the implementation of the getRow method
        template<typename T>
        void impl_getRow(size_t index, T &t) const;

        // Get method to get the data of the provided type at the given index
        template<typename T>
        T get(size_t index) const;
    };

    template<typename... Values>
    void QueryResult::getRow(Values &... values) const {
        // Wraps the implementation method with the starting index of 0 and the expanded variadic
        // values
        impl_getRow(0, values...);
    }

    template<typename T, typename... Values>
    void QueryResult::impl_getRow(size_t index, T &t, Values &... values) const {
        // Sets the T value to the element at the current index
        t = get<T>(index);
        // Calls the recursive case again with the index incremented and the remaining values
        impl_getRow(index + 1, values...);
    }

    template<typename T>
    void QueryResult::impl_getRow(size_t index, T &t) const {
        // In the base case, we just set the T value to the element at the index and don't call
        // any recursive case
        t = get<T>(index);
    }

}

#endif //CONTRACTS_SITE_CLIENT_QUERYRESULT_H
