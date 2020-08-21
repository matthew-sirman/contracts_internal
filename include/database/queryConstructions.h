//
// Created by Matthew.Sirman on 21/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_QUERYCONSTRUCTIONS_H
#define CONTRACTS_SITE_CLIENT_QUERYCONSTRUCTIONS_H

#include <string>
#include <vector>
#include <optional>

#include "QueryResult.h"

namespace sql {

    // Forward declarations
    class SQLSession;

    class TableSelection;

    namespace internal {
        class QueryBuilder;
    }

    // Table
    // Represents a table in the query builder system
    class Table {
        friend class SQLSession;

    public:
        // Enum for the different available types of joins
        enum JoinType {
            INNER,
            LEFT,
            RIGHT
        };

        // Join function to create a join to another table
        Table &join(const std::string &tableName, const std::string &joinFrom, const std::string &joinTo,
                    JoinType joinType = INNER);

        // Select function to select a set of columns from the specified table or set of tables
        template<typename ...T>
        TableSelection select(T &...selections);

    private:
        // Private constructor callable from the SQLSession object
        Table(const std::string &tableName, internal::QueryBuilder &construction);

        // The internal builder object which tracks the details to construct a final SQL string upon request
        internal::QueryBuilder &builder;
    };

    // TableSelection
    // Represents a table selection operation which can be further modified and eventually executed
    class TableSelection {
        friend class Table;

    public:
        // Enum for the ordering direction of any order by sets
        enum OrderDirection {
            ASC,
            DESC
        };

        // Specify a set of where conditions for the query
        template<typename ...T>
        TableSelection &where(T &...conditions);

        // Specify a set of group by conditions for the query
        template<typename ...T>
        TableSelection &groupBy(T &...groupConditions);

        // Specify a set of order by conditions for the query
        template<typename ...T>
        TableSelection &orderBy(OrderDirection direction, T &...orderConditions);

        // Specify a row limit on the returned rows
        TableSelection &limit(size_t n);

        // Executes the constructed query and returns the row results in the form of a QueryResult object
        QueryResult &execute();

    private:
        // Private hidden constructor
        TableSelection(internal::QueryBuilder &construction);

        // The internal builder builder object
        internal::QueryBuilder &builder;
    };

    namespace internal {

        // QueryBuilder
        // A construction object for building an SQL query based on a set of construction methods
        class QueryBuilder {
        public:
            // Constructor
            QueryBuilder(SQLSession *sess);

            // Setter for the root table. This is the table from which any joined tables stem
            void setRootTable(const std::string &tableName);

            // Add a join table to the current query
            void addJoinedTable(const std::string &tableName, const std::string &joinFrom, const std::string &joinOnto,
                                Table::JoinType joinType);

            // Add a set of selections to the selection list
            template<typename ...T>
            void addSelections(T &...selects);

            // Add a set of conditions to the where condition list
            template<typename ...T>
            void addWhereConditions(T &...conditions);

            // Add a set of conditions to the group by condition list
            template<typename ...T>
            void addGroupByConditions(T &...conditions);

            // Add a set of conditions to the order by condition list
            template<typename ...T>
            void addOrderByConditions(TableSelection::OrderDirection direction, T &...conditions);

            // Setter for the limit. The limit defaults to an std::nullopt, implying there is no limit
            void setLimit(size_t lim);

            // Executes the constructed query and returns the results
            QueryResult &execute();

        private:
            // JoinSpec
            // Simple bundling specifier for the information needed for a join
            struct JoinSpec {
                // The table name and two join ON clauses (i.e. JOIN table ON joinFrom=joinOnto)
                std::string table, joinFrom, joinOnto;
                // The type of join to make
                Table::JoinType joinType;

                // Constructor
                inline JoinSpec(const std::string &table, const std::string &joinFrom, const std::string &joinOnto,
                                Table::JoinType joinType)
                        : table(table), joinFrom(joinFrom), joinOnto(joinOnto), joinType(joinType) {

                }
            };

            // OrderSpec
            // Simple bundling specifier for the information needed for an order by clause
            struct OrderSpec {
                // The clause itself
                std::string clause;
                // The direction of the order (i.e. ascending or descending)
                TableSelection::OrderDirection direction;

                // Constructor
                inline OrderSpec(const std::string &clause, TableSelection::OrderDirection direction)
                        : clause(clause), direction(direction) {

                }
            };

            // The information needed to construct the SQL query
            std::string rootTable;
            std::vector<JoinSpec> joins;
            std::vector<std::string> selections;
            std::vector<std::string> whereConditions;
            std::vector<std::string> groupByConditions;
            std::vector<OrderSpec> orderByConditions;
            std::optional<size_t> limit = std::nullopt;

            // The session to call the query on
            SQLSession *sess;

            // Constructs the SQL query string from the builder data
            std::string construct() const;

            // Variadic template recursive case for adding a set of strings to a vector
            template<typename ...T>
            void addAllStrings(std::vector<std::string> &dst, const std::string &condition, T& ...rest);

            // Base case for adding the final string to a vector
            void addAllStrings(std::vector<std::string> &dst, const std::string &condition);

            // Variadic template recursive case for adding a set of order by conditions to the order by conditions
            template<typename ...T>
            void impl_addOrderByConditions(TableSelection::OrderDirection direction, const std::string &condition,
                                           T &...rest);

            // Base case for adding the final order by string and direction to the order by conditions
            void impl_addOrderByConditions(TableSelection::OrderDirection direction, const std::string &condition);
        };

    }

    template<typename... T>
    TableSelection Table::select(T &... selections) {
        // Add the selections to the builder
        builder.addSelections(selections...);
        // Return a newly constructed table selection object
        return TableSelection(builder);
    }

    template<typename... T>
    TableSelection &TableSelection::where(T &... conditions) {
        // Add the where conditions to the builder
        builder.addWhereConditions(conditions...);
        // Return this object - this allows for calling multiple functions on the same line
        return *this;
    }

    template<typename... T>
    TableSelection &TableSelection::groupBy(T &... groupConditions) {
        // Add the group by conditions to the builder
        builder.addGroupByConditions(groupConditions...);
        // Return this object - this allows for calling multiple functions on the same line
        return *this;
    }

    template<typename... T>
    TableSelection &TableSelection::orderBy(TableSelection::OrderDirection direction, T &... orderConditions) {
        // Add the order by conditions to the builder
        builder.addOrderByConditions(direction, orderConditions...);
        // Return this object - this allows for calling multiple functions on the same line
        return *this;
    }

    template<typename... T>
    void internal::QueryBuilder::addSelections(T &... selects) {
        // Call the internal adder to add the "selects" values to the selections vector
        addAllStrings(selections, selects...);
    }

    template<typename... T>
    void internal::QueryBuilder::addWhereConditions(T &... conditions) {
        // Call the internal adder to add the "conditions" values to the where conditions vector
        addAllStrings(whereConditions, conditions...);
    }

    template<typename... T>
    void internal::QueryBuilder::addGroupByConditions(T &... conditions) {
        // Call the internal adder to add the "conditions" values to the group by conditions vector
        addAllStrings(groupByConditions, conditions...);
    }

    template<typename... T>
    void
    internal::QueryBuilder::addOrderByConditions(TableSelection::OrderDirection direction, T &... conditions) {
        // Call the internal adder to add the "conditions" values to the order by conditions vector
        impl_addOrderByConditions(direction, conditions...);
    }

    template<typename... T>
    void internal::QueryBuilder::addAllStrings(std::vector<std::string> &dst, const std::string &condition,
                                               T &... rest) {
        // Add the head string to the destination vector
        dst.push_back(condition);
        // Recurse on the tail
        addAllStrings(dst, rest...);
    }

    template<typename... T>
    void internal::QueryBuilder::impl_addOrderByConditions(TableSelection::OrderDirection direction,
                                                           const std::string &condition, T &... rest) {
        // Add the head string and direction to the order by conditions
        orderByConditions.emplace_back(condition, direction);
        // Recurse on the tail
        impl_addOrderByConditions(direction, rest...);
    }

}

#endif //CONTRACTS_SITE_CLIENT_QUERYCONSTRUCTIONS_H
