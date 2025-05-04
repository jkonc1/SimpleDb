#include "db/expression.h"
#include "db/table.h"

Cell VariableNode::evaluate(const RowReference& row) const {
    return row[name];
}