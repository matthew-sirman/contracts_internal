//
// Created by Matthew.Sirman on 20/08/2020.
//

#include "../../include/database/QueryResult.h"

using namespace sql;

QueryResultRowIterator::QueryResultRowIterator(size_t iterPosition, QueryResult &resultObject)
        : iterPosition(iterPosition), resultObject(resultObject) {

}

Row QueryResultRowIterator::operator*() const {
    // We always simply return the query's current row - the object itself will determine the individual
    // column values to return
    return resultObject.row;
}

QueryResultRowIterator &QueryResultRowIterator::operator++() {
    // Increment the internal iterator
    iterPosition++;
    // Fetch the next row from the query
    resultObject.fetchNextRow();
    // Return the "new" value of the iterator (i.e. after incrementing)
    return *this;
}

QueryResultRowIterator QueryResultRowIterator::operator++(int) {
    // Cache the old value of the iterator before incrementing
    QueryResultRowIterator ret(iterPosition, resultObject);

    // Increment the internal iterator
    iterPosition++;
    // Fetch the next row from the query
    resultObject.fetchNextRow();

    // Return the cached "old" value of the iterator (i.e. before incrementing)
    return ret;
}

bool QueryResultRowIterator::operator==(const QueryResultRowIterator &other) const {
    // Return equal if and only if the iterators are in the same position and pertain to the same query
    return iterPosition == other.iterPosition && &resultObject == &other.resultObject;
}

bool QueryResultRowIterator::operator!=(const QueryResultRowIterator &other) const {
    // Return not equal if and only if the iterators are in different positions or pertain to different queries
    return iterPosition != other.iterPosition || &resultObject != &other.resultObject;
}

QueryResult::QueryResult(SQLSafeHandle<STATEMENT_HANDLE> &&sqlStatementHandle)
        : sqlStatementHandle(std::move(sqlStatementHandle)), columns(*this), row(*this) {

}

void QueryResult::fetchNextRow() {
    // Fetch the next row from the internal SQL statement
    SQLFetch(sqlStatementHandle.get());

    // Increment the current row index
    currentRowIndex++;
}

// Each get method is essentially identical. They all specialise "getting" for a particular type
// through the template argument. Each begins by declaring an appropriate object for the type,
// then calls the internal SQLGetData function to get the data and emplace it in the declared object.
// Finally, the method returns this value.

template<>
Value<char> QueryResult::get<char>(size_t index) const {
    SQLSCHAR result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_CHAR, &result, (SQLLEN) sizeof(SQLSCHAR), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<unsigned char> QueryResult::get<unsigned char>(size_t index) const {
    SQLCHAR result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_CHAR, &result, (SQLLEN) sizeof(SQLCHAR), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<short> QueryResult::get<short>(size_t index) const {
    SQLSMALLINT result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_SMALLINT, &result, (SQLLEN) sizeof(SQLSMALLINT), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<unsigned short> QueryResult::get<unsigned short>(size_t index) const {
    SQLUSMALLINT result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_SMALLINT, &result, (SQLLEN) sizeof(SQLUSMALLINT), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<int> QueryResult::get<int>(size_t index) const {
    SQLINTEGER result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_INTEGER, &result, (SQLLEN) sizeof(SQLINTEGER), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<unsigned int> QueryResult::get<unsigned int>(size_t index) const {
    SQLUINTEGER result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_INTEGER, &result, (SQLLEN) sizeof(SQLUINTEGER), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<long long> QueryResult::get<long long>(size_t index) const {
    SQLBIGINT result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_INTEGER, &result, (SQLLEN) sizeof(SQLINTEGER), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<unsigned long long> QueryResult::get<unsigned long long>(size_t index) const {
    SQLUBIGINT result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_INTEGER, &result, (SQLLEN) sizeof(SQLUBIGINT), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<float> QueryResult::get<float>(size_t index) const {
    SQLREAL result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_REAL, &result, (SQLLEN) sizeof(SQLREAL), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<double> QueryResult::get<double>(size_t index) const {
    SQLDOUBLE result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_DOUBLE, &result, (SQLLEN) sizeof(SQLDOUBLE), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<std::string> QueryResult::get<std::string>(size_t index) const {
    // This is a differing get method.

    // First we declare an array for the result. This is declared in static storage
    static SQLCHAR result[MAX_QUERY_STRING_LENGTH];
    // Declare a value to hold the length of the returned string (usually this is omitted as we already know the
    // size for other types)
    SQLLEN resultLength = 0;

    // Then, we call the SQLGetData function as usual, which writes the string to the result buffer and the length
    // to the resultLength value
    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_CHAR, result, (SQLLEN) sizeof(result), &resultLength);

    if (resultLength == SQL_NULL_DATA) {
        return null_value;
    }

    // Finally, we construct a string from the result buffer and length. This implicitly copies from the buffer,
    // so it does not matter that the buffer itself is in static space
    return std::string((const char *) result, resultLength);
}

template<>
Value<bool> QueryResult::get<bool>(size_t index) const {
    SQLCHAR result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, SQL_CHAR, &result, (SQLLEN) sizeof(SQLCHAR), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return result;
}

template<>
Value<Date> QueryResult::get<Date>(size_t index) const {
    SQL_DATE_STRUCT result;
    SQLLEN length;

    SQLGetData(sqlStatementHandle.get(), index + 1, 91, &result, (SQLLEN) sizeof(SQL_DATE_STRUCT), &length);

    if (length == SQL_NULL_DATA) {
        return null_value;
    }

    return Date(result.year, result.month, result.day);
}

template<>
Value<Price> QueryResult::get<Price>(size_t index) const {
    // Get the value as a float
    Value<float> value = get<float>(index);

    if (!value.hasValue()) {
        return null_value;
    }

    // Return a price of this value
    return Price(value.value());
}

size_t QueryResult::rowCount() const {
    // Declare an SQLLEN object to hold the row count
    SQLLEN rowCount;

    // Call the internal SQLRowCount function
    SQLRowCount(sqlStatementHandle.get(), &rowCount);

    // Return the resultant row count value
    return rowCount;
}

size_t QueryResult::currentRow() const {
    // Simple getter for the current row index variable
    return currentRowIndex;
}

QueryResultRowIterator QueryResult::begin() {
    // Fetch the first row - the rows are incremented whenever the increment operator is called on the iterator,
    // but it will not be called for the first row, hence we manually increment it here as begin is called.
    fetchNextRow();
    // Return an iterator in position 0
    return QueryResultRowIterator(0, *this);
}

QueryResultRowIterator QueryResult::end() {
    // Return an iterator with the position of rowCount, meaning it will be the "final" row
    return QueryResultRowIterator(rowCount(), *this);
}
