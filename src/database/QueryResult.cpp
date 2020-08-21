//
// Created by Matthew.Sirman on 20/08/2020.
//

#include "../../include/database/QueryResult.h"

using namespace sql;

QueryResultRowIterator::QueryResultRowIterator(size_t iterPosition, QueryResult *resultObject)
        : iterPosition(iterPosition), resultObject(resultObject) {

}

Row QueryResultRowIterator::operator*() const {
    return resultObject->row;
}

QueryResultRowIterator &QueryResultRowIterator::operator++() {
    iterPosition++;
    resultObject->fetchNextRow();
    return *this;
}

QueryResultRowIterator QueryResultRowIterator::operator++(int) {
    QueryResultRowIterator ret(iterPosition, resultObject);

    iterPosition++;
    resultObject->fetchNextRow();

    return ret;
}

bool QueryResultRowIterator::operator==(const QueryResultRowIterator &other) const {
    return iterPosition == other.iterPosition;
}

bool QueryResultRowIterator::operator!=(const QueryResultRowIterator &other) const {
    return iterPosition != other.iterPosition;
}

QueryResult::QueryResult(SQLHANDLE sqlStatementHandle)
        : sqlStatementHandle(sqlStatementHandle), columns(this), row(this) {

}

void QueryResult::fetchNextRow() {
    // Fetches the next row from the internal SQL statement
    SQLFetch(sqlStatementHandle);

    currentRowIndex++;
}

// Each get method is essentially identical. They all specialise "getting" for a particular type
// through the template argument. Each begins by declaring an appropriate object for the type,
// then calls the internal SQLGetData function to get the data and emplace it in the declared object.
// Finally, the method returns this value.

template<>
char QueryResult::get<char>(size_t index) const {
    SQLSCHAR result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_CHAR, &result, (SQLLEN) sizeof(SQLSCHAR), nullptr);

    return result;
}

template<>
unsigned char QueryResult::get<unsigned char>(size_t index) const {
    SQLCHAR result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_CHAR, &result, (SQLLEN) sizeof(SQLCHAR), nullptr);

    return result;
}

template<>
short QueryResult::get<short>(size_t index) const {
    SQLSMALLINT result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_SMALLINT, &result, (SQLLEN) sizeof(SQLSMALLINT), nullptr);

    return result;
}

template<>
unsigned short QueryResult::get<unsigned short>(size_t index) const {
    SQLUSMALLINT result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_SMALLINT, &result, (SQLLEN) sizeof(SQLUSMALLINT), nullptr);

    return result;
}

template<>
int QueryResult::get<int>(size_t index) const {
    SQLINTEGER result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_INTEGER, &result, (SQLLEN) sizeof(SQLINTEGER), nullptr);

    return result;
}

template<>
unsigned int QueryResult::get<unsigned int>(size_t index) const {
    SQLUINTEGER result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_INTEGER, &result, (SQLLEN) sizeof(SQLUINTEGER), nullptr);

    return result;
}

template<>
long long QueryResult::get<long long>(size_t index) const {
    SQLBIGINT result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_INTEGER, &result, (SQLLEN) sizeof(SQLINTEGER), nullptr);

    return result;
}

template<>
unsigned long long QueryResult::get<unsigned long long>(size_t index) const {
    SQLUBIGINT result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_INTEGER, &result, (SQLLEN) sizeof(SQLUBIGINT), nullptr);

    return result;
}

template<>
float QueryResult::get<float>(size_t index) const {
    SQLREAL result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_REAL, &result, (SQLLEN) sizeof(SQLREAL), nullptr);

    return result;
}

template<>
double QueryResult::get<double>(size_t index) const {
    SQLDOUBLE result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_DOUBLE, &result, (SQLLEN) sizeof(SQLDOUBLE), nullptr);

    return result;
}

template<>
std::string QueryResult::get<std::string>(size_t index) const {
    // This is the only differing get method.

    // First we declare an array for the result. This is declared in static storage
    static SQLCHAR result[MAX_QUERY_STRING_LENGTH];
    // Declare a value to hold the length of the returned string (usually this is omitted as we already know the
    // size for other types)
    SQLLEN resultLength = 0;

    // Then, we call the SQLGetData function as usual, which writes the string to the result buffer and the length
    // to the resultLength value
    SQLGetData(sqlStatementHandle, index + 1, SQL_CHAR, result, (SQLLEN) sizeof(result), &resultLength);

    // Finally, we construct a string from the result buffer and length. This implicitly copies from the buffer,
    // so it does not matter that the buffer itself is in static space
    return std::string((const char *) result, resultLength);
}

template<>
bool QueryResult::get<bool>(size_t index) const {
    SQLCHAR result;

    SQLGetData(sqlStatementHandle, index + 1, SQL_CHAR, &result, (SQLLEN) sizeof(SQLCHAR), nullptr);

    return result;
}

size_t QueryResult::rowCount() const {
    // Declare an SQLLEN object to hold the row count
    SQLLEN rowCount;

    // Call the internal SQLRowCount function
    SQLRowCount(sqlStatementHandle, &rowCount);

    // Return the resultant row count value
    return rowCount;
}

size_t QueryResult::currentRow() const {
    // Simple getter for the current row index variable
    return currentRowIndex;
}

QueryResultRowIterator QueryResult::begin() {
    fetchNextRow();
    return QueryResultRowIterator(0, this);
}

QueryResultRowIterator QueryResult::end() {
    return QueryResultRowIterator(rowCount(), this);
}
