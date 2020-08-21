//
// Created by Matthew.Sirman on 21/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_QUERYCONSTRUCTIONS_H
#define CONTRACTS_SITE_CLIENT_QUERYCONSTRUCTIONS_H

#include <string>
#include <vector>
#include <optional>

#include "QueryResult.h"

class SageDatabaseManager;

namespace sql {

    class TableSelection;

    namespace internal {
        class SelectionConstruction;
    }

    class Table {
        friend class ::SageDatabaseManager;

    public:
        enum JoinType {
            INNER,
            LEFT,
            RIGHT
        };

        Table &join(const std::string &tableName, const std::string &joinFrom, const std::string &joinTo,
                    JoinType joinType = INNER);

        template<typename ...T>
        TableSelection select(T &...selections);

    private:
        Table(const std::string &tableName, internal::SelectionConstruction &construction);

        internal::SelectionConstruction &construction;
    };

    class TableSelection {
        friend class Table;

    public:
        enum OrderDirection {
            ASC,
            DESC
        };

        template<typename ...T>
        TableSelection &where(T &...conditions);

        template<typename ...T>
        TableSelection &groupBy(T &...groupConditions);

        template<typename ...T>
        TableSelection &orderBy(OrderDirection direction, T &...orderConditions);

        TableSelection &limit(size_t n);

        QueryResult execute();

    private:
        TableSelection(internal::SelectionConstruction &construction);

        internal::SelectionConstruction &construction;
    };

    namespace internal {

        class SelectionConstruction {
        public:
            SelectionConstruction(SageDatabaseManager *manager);

            void setRootTable(const std::string &tableName);

            void addJoinedTable(const std::string &tableName, const std::string &joinFrom, const std::string &joinOnto,
                                Table::JoinType joinType);

            template<typename ...T>
            void addSelections(T &...selects);

            template<typename ...T>
            void addWhereConditions(T &...conditions);

            template<typename ...T>
            void addGroupByConditions(T &...conditions);

            template<typename ...T>
            void addOrderByConditions(TableSelection::OrderDirection direction, T &...conditions);

            void setLimit(size_t lim);

            QueryResult execute();

        private:
            struct JoinSpec {
                std::string table, joinFrom, joinOnto;
                Table::JoinType joinType;

                inline JoinSpec(const std::string &table, const std::string &joinFrom, const std::string &joinOnto,
                                Table::JoinType joinType)
                        : table(table), joinFrom(joinFrom), joinOnto(joinOnto), joinType(joinType) {

                }
            };

            struct OrderSpec {
                std::string clause;
                TableSelection::OrderDirection direction;

                inline OrderSpec(const std::string &clause, TableSelection::OrderDirection direction)
                        : clause(clause), direction(direction) {

                }
            };

            std::string rootTable;
            std::vector<JoinSpec> joins;
            std::vector<std::string> selections;
            std::vector<std::string> whereConditions;
            std::vector<std::string> groupByConditions;
            std::vector<OrderSpec> orderByConditions;
            std::optional<size_t> limit = std::nullopt;

            SageDatabaseManager *manager;

            std::string construct() const;

            template<typename ...T>
            void addAllStrings(std::vector<std::string> &dst, const std::string &condition, T& ...rest);

            void addAllStrings(std::vector<std::string> &dst, const std::string &condition);

            template<typename ...T>
            void impl_addOrderByConditions(TableSelection::OrderDirection direction, const std::string &condition,
                                           T &...rest);

            void impl_addOrderByConditions(TableSelection::OrderDirection direction, const std::string &condition);
        };

    }

    template<typename... T>
    TableSelection Table::select(T &... selections) {
        construction.addSelections(selections...);
        return TableSelection(construction);
    }

    template<typename... T>
    TableSelection &TableSelection::where(T &... conditions) {
        construction.addWhereConditions(conditions...);
        return *this;
    }

    template<typename... T>
    TableSelection &TableSelection::groupBy(T &... groupConditions) {
        construction.addGroupByConditions(groupConditions...);
        return *this;
    }

    template<typename... T>
    TableSelection &TableSelection::orderBy(TableSelection::OrderDirection direction, T &... orderConditions) {
        construction.addOrderByConditions(direction, orderConditions...);
        return *this;
    }

    template<typename... T>
    void internal::SelectionConstruction::addSelections(T &... selects) {
        addAllStrings(selections, selects...);
    }

    template<typename... T>
    void internal::SelectionConstruction::addWhereConditions(T &... conditions) {
        addAllStrings(whereConditions, conditions...);
    }

    template<typename... T>
    void internal::SelectionConstruction::addGroupByConditions(T &... conditions) {
        addAllStrings(groupByConditions, conditions...);
    }

    template<typename... T>
    void
    internal::SelectionConstruction::addOrderByConditions(TableSelection::OrderDirection direction, T &... conditions) {
        impl_addOrderByConditions(direction, conditions...);
    }

    template<typename... T>
    void internal::SelectionConstruction::addAllStrings(std::vector<std::string> &dst, const std::string &condition,
                                                        T &... rest) {
        dst.push_back(condition);
        addAllStrings(dst, rest...);
    }

    template<typename... T>
    void internal::SelectionConstruction::impl_addOrderByConditions(TableSelection::OrderDirection direction,
                                                                    const std::string &condition, T &... rest) {
        orderByConditions.emplace_back(condition, direction);
        impl_addOrderByConditions(direction, rest...);
    }

}

#endif //CONTRACTS_SITE_CLIENT_QUERYCONSTRUCTIONS_H
