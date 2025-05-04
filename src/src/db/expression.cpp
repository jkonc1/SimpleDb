#include "db/expression.h"
#include "db/table.h"
#include "db/variable_list.h"

Cell VariableNode::evaluate(const VariableList& row) const {
    return row.get_value(name);
}

Cell::DataType VariableNode::get_type(const VariableList& row) const {
    return row.get_type(name);
}
