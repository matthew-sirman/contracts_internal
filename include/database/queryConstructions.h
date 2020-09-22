//
// Created by Matthew.Sirman on 21/08/2020.
//

#ifndef CONTRACTS_INTERNAL_QUERYCONSTRUCTIONS_H
#define CONTRACTS_INTERNAL_QUERYCONSTRUCTIONS_H

#include <string>
#include <utility>
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
    // Enum for the different available types of joins
    enum class JoinType {
        INNER,
        LEFT,
        RIGHT
    };

    // Enum for the ordering direction of any order by sets
    enum class OrderDirection {
        ASC,
        DESC
    };

    // Table
    // Represents a table in the query builder system
    class Table {
        friend class SQLSession;

    public:

        // Join function to create a join to another table
        Table &join(const std::string &tableName, const std::string &joinFrom, const std::string &joinTo,
                    JoinType joinType = JoinType::INNER);

        // Join function with alias
        Table &join(const std::string &tableName, const std::string &tableAlias, const std::string &joinFrom,
                    const std::string &joinTo, JoinType joinType = JoinType::INNER);

        // Select function to select a set of columns from the specified table or set of tables
        template<typename ...T>
        TableSelection select(T &&...selections);

    private:
        // Private constructor callable from the SQLSession object
        Table(const std::string &tableName, const std::optional<std::string> &tableAlias, std::unique_ptr<internal::QueryBuilder> construction);

        // The internal builder object which tracks the details to construct a final SQL string upon request
        std::unique_ptr<internal::QueryBuilder> builder;
    };

    // TableSelection
    // Represents a table selection operation which can be further modified and eventually executed
    class TableSelection {
        friend class Table;

    public:

        // Specify a set of where conditions for the query
        template<typename ...T>
        TableSelection &where(T &&...conditions);

        // Specify a set of group by conditions for the query
        template<typename ...T>
        TableSelection &groupBy(T &&...groupConditions);

        // Specify a set of order by conditions for the query
        template<typename ...T>
        TableSelection &orderBy(OrderDirection direction, T &&...orderConditions);

        // Specify a row limit on the returned rows
        TableSelection &limit(size_t n);

        // Executes the constructed query and returns the row results in the form of a QueryResult object
        QueryResult execute();

        // Executes the constructed query as an Oracle-Style query
        QueryResult executeOracle();

    private:
        // Private hidden constructor
        explicit TableSelection(std::unique_ptr<internal::QueryBuilder> builder);

        // The internal builder builder object
        std::unique_ptr<internal::QueryBuilder> builder;
    };

    namespace internal {

        // QueryBuilder
        // A construction object for building an SQL query based on a set of construction methods
        class QueryBuilder {
        public:
            // Constructor
            explicit QueryBuilder(SQLSession *sess);

            // Setter for the root table. This is the table from which any joined tables stem
            void setRootTable(const std::string &tableName, const std::optional<std::string> &tableAlias);

            // Add a join table to the current query
            void addJoinedTable(const std::string &tableName, const std::optional<std::string> &tableAlias,
                                const std::string &joinFrom, const std::string &joinOnto,
                                JoinType joinType);

            // Add a set of selections to the selection list
            template<typename ...T>
            void addSelections(T &&...selects);

            // Add a set of conditions to the where condition list
            template<typename ...T>
            void addWhereConditions(T &&...conditions);

            // Add a set of conditions to the group by condition list
            template<typename ...T>
            void addGroupByConditions(T &&...conditions);

            // Add a set of conditions to the order by condition list
            template<typename ...T>
            void addOrderByConditions(OrderDirection direction, T &&...conditions);

            // Setter for the limit. The limit defaults to an std::nullopt, implying there is no limit
            void setLimit(size_t lim);

            // Executes the constructed query and returns the results
            QueryResult execute();

            // Executes the constructed query as an Oracle-Style query and returns the results
            QueryResult executeOracle();

        private:
            // JoinSpec
            // Simple bundling specifier for the information needed for a join
            struct JoinSpec {
                // The table name and two join ON clauses (i.e. JOIN table ON joinFrom=joinOnto)
                std::string table, joinFrom, joinOnto;
                std::optional<std::string> tableAlias;
                // The type of join to make
                JoinType joinType;

                // Constructor
                inline JoinSpec(std::string table, std::optional<std::string> tableAlias, std::string joinFrom, std::string joinOnto,
                                JoinType joinType)
                        : table(std::move(table)), tableAlias(std::move(tableAlias)), joinFrom(std::move(joinFrom)),
                          joinOnto(std::move(joinOnto)), joinType(joinType) {

                }
            };

            // OrderSpec
            // Simple bundling specifier for the information needed for an order by clause
            struct OrderSpec {
                // The clause itself
                std::string clause;
                // The direction of the order (i.e. ascending or descending)
                OrderDirection direction;

                // Constructor
                inline OrderSpec(std::string clause, OrderDirection direction)
                        : clause(std::move(clause)), direction(direction) {

                }
            };

            // The information needed to construct the SQL query
            std::string rootTable;
            std::optional<std::string> rootTableAlias;
            std::vector<JoinSpec> joins;
            std::vector<std::string> selections;
            std::vector<std::string> whereConditions;
            std::vector<std::string> groupByConditions;
            std::vector<OrderSpec> orderByConditions;
            std::optional<size_t> limit = std::nullopt;

            // The session to call the query on
            SQLSession *sess;

            // Construct the SQL query string from the builder data
            std::string construct() const;

            // Construct the SQL query string in Oracle style SQL
            std::string constructOracle() const;

            // Variadic template recursive case for adding a set of strings to a vector
            template<typename ...T>
            void addAllStrings(std::vector<std::string> &dst, const std::string &condition, T&& ...rest);

            // Base case for adding the final string to a vector
            void addAllStrings(std::vector<std::string> &dst, const std::string &condition);

            // Variadic template recursive case for adding a set of order by conditions to the order by conditions
            template<typename ...T>
            void impl_addOrderByConditions(OrderDirection direction, const std::string &condition,
                                           T &&...rest);

            // Base case for adding the final order by string and direction to the order by conditions
            void impl_addOrderByConditions(OrderDirection direction, const std::string &condition);
        };

    }

    template<typename... T>
    TableSelection Table::select(T &&... selections) {
        // Add the selections to the builder
        builder->addSelections(selections...);
        // Return a newly constructed table selection object
        return TableSelection(std::move(builder));
    }

    template<typename... T>
    TableSelection &TableSelection::where(T &&... conditions) {
        // Add the where conditions to the builder
        builder->addWhereConditions(conditions...);
        // Return this object - this allows for calling multiple functions on the same line
        return *this;
    }

    template<typename... T>
    TableSelection &TableSelection::groupBy(T &&... groupConditions) {
        // Add the group by conditions to the builder
        builder->addGroupByConditions(groupConditions...);
        // Return this object - this allows for calling multiple functions on the same line
        return *this;
    }

    template<typename... T>
    TableSelection &TableSelection::orderBy(OrderDirection direction, T &&... orderConditions) {
        // Add the order by conditions to the builder
        builder->addOrderByConditions(direction, orderConditions...);
        // Return this object - this allows for calling multiple functions on the same line
        return *this;
    }

    template<typename... T>
    void internal::QueryBuilder::addSelections(T &&... selects) {
        // Call the internal adder to add the "selects" values to the selections vector
        addAllStrings(selections, selects...);
    }

    template<typename... T>
    void internal::QueryBuilder::addWhereConditions(T &&... conditions) {
        // Call the internal adder to add the "conditions" values to the where conditions vector
        addAllStrings(whereConditions, conditions...);
    }

    template<typename... T>
    void internal::QueryBuilder::addGroupByConditions(T &&... conditions) {
        // Call the internal adder to add the "conditions" values to the group by conditions vector
        addAllStrings(groupByConditions, conditions...);
    }

    template<typename... T>
    void
    internal::QueryBuilder::addOrderByConditions(OrderDirection direction, T &&... conditions) {
        // Call the internal adder to add the "conditions" values to the order by conditions vector
        impl_addOrderByConditions(direction, conditions...);
    }

    template<typename... T>
    void internal::QueryBuilder::addAllStrings(std::vector<std::string> &dst, const std::string &condition,
                                               T &&... rest) {
        // Add the head string to the destination vector
        dst.push_back(condition);
        // Recurse on the tail
        addAllStrings(dst, rest...);
    }

    template<typename... T>
    void internal::QueryBuilder::impl_addOrderByConditions(OrderDirection direction,
                                                           const std::string &condition, T &&... rest) {
        // Add the head string and direction to the order by conditions
        orderByConditions.emplace_back(condition, direction);
        // Recurse on the tail
        impl_addOrderByConditions(direction, rest...);
    }

}

#endif //CONTRACTS_INTERNAL_QUERYCONSTRUCTIONS_H
