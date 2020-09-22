//
// Created by Matthew.Sirman on 21/08/2020.
//

#include "../../include/database/queryConstructions.h"
#include "../../include/database/SQLSession.h"

using namespace sql;
using namespace sql::internal;

Table &Table::join(const std::string &tableName, const std::string &joinFrom, const std::string &joinTo,
                   JoinType joinType) {
    // Add the joined table to the builder
    builder->addJoinedTable(tableName, std::nullopt, joinFrom, joinTo, joinType);
    // Return this object - this allows for calling multiple functions on the same line
    return *this;
}

Table &Table::join(const std::string &tableName, const std::string &tableAlias, const std::string &joinFrom,
                   const std::string &joinTo, JoinType joinType) {
    // Add the joined table to the builder
    builder->addJoinedTable(tableName, tableAlias, joinFrom, joinTo, joinType);
    // Return this object - this allows for calling multiple functions on the same line
    return *this;
}

Table::Table(const std::string &tableName, const std::optional<std::string> &tableAlias, std::unique_ptr<QueryBuilder> construction)
        : builder(std::move(construction)) {
    // Set the root table. This is the first piece of information to be set
    builder->setRootTable(tableName, tableAlias);
}

TableSelection &TableSelection::limit(size_t n) {
    builder->setLimit(n);
    // Return this object - this allows for calling multiple functions on the same line
    return *this;
}

QueryResult TableSelection::execute() {
    // Return builder's executed query result
    return builder->execute();
}

QueryResult TableSelection::executeOracle() {
    return builder->executeOracle();
}

TableSelection::TableSelection(std::unique_ptr<QueryBuilder> construction)
     : builder(std::move(construction)) {

}

QueryBuilder::QueryBuilder(SQLSession *sess)
    : sess(sess) {

}

void QueryBuilder::setRootTable(const std::string &tableName, const std::optional<std::string> &tableAlias) {
    // Set the internal root table to the first table
    rootTable = tableName;
    rootTableAlias = tableAlias;
}

void QueryBuilder::addJoinedTable(const std::string &tableName, const std::optional<std::string> &tableAlias,
                                  const std::string &joinFrom, const std::string &joinOnto, JoinType joinType) {
    // Emplace the join onto the back of the joins list
    joins.emplace_back(tableName, tableAlias, joinFrom, joinOnto, joinType);
}

void QueryBuilder::setLimit(size_t lim) {
    // Set the internal limit optional to the passed in limit - it will no longer be a nullopt
    // and thus included in the query
    limit = lim;
}

QueryResult QueryBuilder::execute() {
    // Executes the actual query on the session and returns the results
    return sess->executeQuery(construct());
}

QueryResult QueryBuilder::executeOracle() {
    return sess->executeQuery(constructOracle());
}

std::string QueryBuilder::construct() const {
    // Declare a string stream to write the query to
    std::stringstream sql;

    // Always begins with a SELECT statement
    sql << "SELECT ";

    // If there is a limit, add a TOP statement to the query
    if (limit.has_value()) {
        sql << "TOP " << limit.value() << " ";
    }

    // For every column we wish to select, add them to the query string separated by commas
    for (std::vector<std::string>::const_iterator it = selections.begin(); it != selections.end(); it++) {
        sql << *it;
        if (it != selections.end() - 1) {
            sql << ", ";
        }
    }
    sql << std::endl;

    // Add a FROM clause for the root table
    sql << "FROM " << rootTable;
    if (rootTableAlias.has_value()) {
        sql << " " << rootTableAlias.value();
    }
    sql << std::endl;

    // For every joined table (which may be none)
    for (const JoinSpec &join : joins) {
        // Specify the type of join based on the specification
        switch (join.joinType) {
            case JoinType::INNER:
                sql << "INNER JOIN ";
                break;
            case JoinType::LEFT:
                sql << "LEFT JOIN ";
                break;
            case JoinType::RIGHT:
                sql << "RIGHT JOIN ";
                break;
        }

        // Add the ON filtering clause based on the specification
        sql << join.table;
        if (join.tableAlias.has_value()) {
            sql << " " << join.tableAlias.value();
        }
        sql << " ON " << join.joinFrom << "=" << join.joinOnto << std::endl;
    }

    // If there are any where conditions
    if (!whereConditions.empty()) {
        // Add the WHERE clause
        sql << "WHERE ";

        // For every where condition
        for (std::vector<std::string>::const_iterator it = whereConditions.begin(); it != whereConditions.end(); it++) {
            // Add the condition string to the query in a comma separated list
            sql << *it;
            if (it != whereConditions.end() - 1) {
                sql << " AND ";
            }
        }

        sql << std::endl;
    }

    // If there are any order by conditions
    if (!orderByConditions.empty()) {
        // Add the ORDER BY clause
        sql << "ORDER BY ";

        // For every order by condition
        for (std::vector<OrderSpec>::const_iterator it = orderByConditions.begin(); it != orderByConditions.end(); it++) {
            // Add the condition string to the query
            sql << it->clause << " ";
            // Add the direction based on the specification
            switch (it->direction) {
                case OrderDirection::ASC:
                    sql << "ASC";
                    break;
                case OrderDirection::DESC:
                    sql << "DESC";
                    break;
            }

            if (it != orderByConditions.end() - 1) {
                sql << ", ";
            }
        }

        sql << std::endl;
    }

    // If there are any group by conditions
    if (!groupByConditions.empty()) {
        // Add the GROUP BY clause
        sql << "GROUP BY ";

        // For every group by condition
        for (std::vector<std::string>::const_iterator it = groupByConditions.begin(); it != groupByConditions.end(); it++) {
            // Add the condition string to the query in a comma separated list
            sql << *it;

            if (it != groupByConditions.end() - 1) {
                sql << ", ";
            }
        }

        sql << std::endl;
    }

    // Finally, after we have constructed the query string in the string stream, return the constructed string
    return sql.str();
}

std::string QueryBuilder::constructOracle() const {
    // Declare a string stream to write the query to
    std::stringstream sql;

    // Always begins with a SELECT statement
    sql << "SELECT ";

    // If there is a limit, add a TOP statement to the query
    if (limit.has_value()) {
        sql << "TOP " << limit.value() << " ";
    }

    // For every column we wish to select, add them to the query string separated by commas
    for (std::vector<std::string>::const_iterator it = selections.begin(); it != selections.end(); it++) {
        sql << *it;
        if (it != selections.end() - 1) {
            sql << ", ";
        }
    }
    sql << std::endl;

    // Add a FROM clause for the root table
    sql << "FROM " << rootTable;
    if (rootTableAlias.has_value()) {
        sql << " " << rootTableAlias.value();
    }

    // For every joined table (which may be none)
    for (const JoinSpec &join : joins) {
        sql << ", " << join.table;
        if (join.tableAlias.has_value()) {
            sql << " " << join.tableAlias.value();
        }
    }

    sql << std::endl;

    if (!joins.empty() || !whereConditions.empty()) {
        sql << "WHERE ";
    }

    for (std::vector<JoinSpec>::const_iterator it = joins.begin(); it != joins.end(); it++) {
        switch (it->joinType) {
            case JoinType::INNER:
                sql << it->joinFrom << "=" << it->joinOnto;
                break;
            case JoinType::LEFT:
                sql << it->joinFrom << "=" << it->joinOnto << "(+)";
                break;
            case JoinType::RIGHT:
                sql << it->joinFrom << "(+)=" << it->joinOnto;
                break;
        }

        if (it != joins.end() - 1 || !whereConditions.empty()) {
            sql << " AND" << std::endl;
        }
    }

    // For every where condition
    for (std::vector<std::string>::const_iterator it = whereConditions.begin(); it != whereConditions.end(); it++) {
        // Add the condition string to the query in a comma separated list
        sql << *it;
        if (it != whereConditions.end() - 1) {
            sql << " AND ";
        }
    }

    sql << std::endl;

    // If there are any order by conditions
    if (!orderByConditions.empty()) {
        // Add the ORDER BY clause
        sql << "ORDER BY ";

        // For every order by condition
        for (std::vector<OrderSpec>::const_iterator it = orderByConditions.begin(); it != orderByConditions.end(); it++) {
            // Add the condition string to the query
            sql << it->clause << " ";
            // Add the direction based on the specification
            switch (it->direction) {
                case OrderDirection::ASC:
                    sql << "ASC";
                    break;
                case OrderDirection::DESC:
                    sql << "DESC";
                    break;
            }

            if (it != orderByConditions.end() - 1) {
                sql << ", ";
            }
        }

        sql << std::endl;
    }

    // If there are any group by conditions
    if (!groupByConditions.empty()) {
        // Add the GROUP BY clause
        sql << "GROUP BY ";

        // For every group by condition
        for (std::vector<std::string>::const_iterator it = groupByConditions.begin(); it != groupByConditions.end(); it++) {
            // Add the condition string to the query in a comma separated list
            sql << *it;

            if (it != groupByConditions.end() - 1) {
                sql << ", ";
            }
        }

        sql << std::endl;
    }

    // Finally, after we have constructed the query string in the string stream, return the constructed string
    return sql.str();
}

void QueryBuilder::addAllStrings(std::vector<std::string> &dst, const std::string &condition) {
    // Base case for the addAllStrings method - add the final string and don't recurse
    dst.push_back(condition);
}

void QueryBuilder::impl_addOrderByConditions(OrderDirection direction,
                                             const std::string &condition) {
    // Base case for the add order by conditions method - add the final order by and don't recurse
    orderByConditions.emplace_back(condition, direction);
}
