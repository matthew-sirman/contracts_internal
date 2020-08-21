//
// Created by Matthew.Sirman on 21/08/2020.
//

#include "../../include/database/queryConstructions.h"
#include "../../include/database/SageDatabaseManager.h"

using namespace sql;
using namespace sql::internal;

Table &Table::join(const std::string &tableName, const std::string &joinFrom, const std::string &joinTo,
                   Table::JoinType joinType) {
    construction.addJoinedTable(tableName, joinFrom, joinTo, joinType);
    return *this;
}

Table::Table(const std::string &tableName, SelectionConstruction &construction)
        : construction(construction) {
    construction.setRootTable(tableName);
}

TableSelection &TableSelection::limit(size_t n) {
    construction.setLimit(n);
    return *this;
}

QueryResult TableSelection::execute() {
    return construction.execute();
}

TableSelection::TableSelection(SelectionConstruction &construction)
     : construction(construction) {

}

SelectionConstruction::SelectionConstruction(SageDatabaseManager *manager)
    : manager(manager) {

}

void SelectionConstruction::setRootTable(const std::string &tableName) {
    rootTable = tableName;
}

void SelectionConstruction::addJoinedTable(const std::string &tableName, const std::string &joinFrom,
                                           const std::string &joinOnto, Table::JoinType joinType) {
    joins.emplace_back(tableName, joinFrom, joinOnto, joinType);
}

void SelectionConstruction::setLimit(size_t lim) {
    limit = lim;
}

QueryResult SelectionConstruction::execute() {
    return manager->executeQuery(construct());
}

#include <iostream>

std::string SelectionConstruction::construct() const {
    std::stringstream sql;

    sql << "SELECT ";

    if (limit.has_value()) {
        sql << "TOP " << limit.value() << " ";
    }

    for (std::vector<std::string>::const_iterator it = selections.begin(); it != selections.end(); it++) {
        sql << *it;
        if (it != selections.end() - 1) {
            sql << ", ";
        }
    }
    sql << std::endl;

    sql << "FROM " << rootTable << std::endl;

    for (const JoinSpec &join : joins) {
        switch (join.joinType) {
            case Table::INNER:
                sql << "INNER JOIN ";
                break;
            case Table::LEFT:
                sql << "LEFT JOIN ";
                break;
            case Table::RIGHT:
                sql << "RIGHT JOIN ";
                break;
        }

        sql << join.table << " ON " << join.joinFrom << "=" << join.joinOnto << std::endl;
    }

    if (!whereConditions.empty()) {
        sql << "WHERE ";

        for (std::vector<std::string>::const_iterator it = whereConditions.begin(); it != whereConditions.end(); it++) {
            sql << *it;
            if (it != whereConditions.end() - 1) {
                sql << ", ";
            }
        }

        sql << std::endl;
    }

    if (!orderByConditions.empty()) {
        sql << "ORDER BY ";

        for (std::vector<OrderSpec>::const_iterator it = orderByConditions.begin(); it != orderByConditions.end(); it++) {
            sql << it->clause << " ";
            switch (it->direction) {
                case sql::TableSelection::ASC:
                    sql << "ASC";
                    break;
                case sql::TableSelection::DESC:
                    sql << "DESC";
                    break;
            }

            if (it != orderByConditions.end() - 1) {
                sql << ", ";
            }
        }

        sql << std::endl;
    }

    if (!groupByConditions.empty()) {
        sql << "GROUP BY ";

        for (std::vector<std::string>::const_iterator it = groupByConditions.begin(); it != groupByConditions.end(); it++) {
            sql << *it;

            if (it != groupByConditions.end() - 1) {
                sql << ", ";
            }
        }

        sql << std::endl;
    }

    std::cout << std::endl << "######" << std::endl << sql.str() << std::endl<< "####" << std::endl;

    return sql.str();
}

void SelectionConstruction::addAllStrings(std::vector<std::string> &dst, const std::string &condition) {
    dst.push_back(condition);
}

void SelectionConstruction::impl_addOrderByConditions(TableSelection::OrderDirection direction,
                                                      const std::string &condition) {
    orderByConditions.emplace_back(condition, direction);
}
