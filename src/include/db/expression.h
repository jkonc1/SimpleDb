#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "db/cell.h"

#include <valarray>
#include <memory>

using CellVector = std::valarray<Cell>;

struct EvaluatedExpression {
    Cell::DataType type;
    CellVector values;
};

class VariableList;

class ExpressionNode{
public:
    virtual Cell evaluate(const VariableList& row) const = 0;
    virtual Cell::DataType get_type(const VariableList& row) const = 0;
    
    ExpressionNode() = default;
    ExpressionNode(ExpressionNode&&) = default;
    ExpressionNode(const ExpressionNode&) = delete;
    ExpressionNode& operator=(ExpressionNode&&) = default;
    ExpressionNode& operator=(const ExpressionNode&) = delete;
    
    virtual ~ExpressionNode() = default;
};

template <class OP>
class BinaryOperationNode : public ExpressionNode{
public:
    BinaryOperationNode(std::unique_ptr<ExpressionNode> left, std::unique_ptr<ExpressionNode> right) : left(std::move(left)), right(std::move(right)) {}

    Cell evaluate([[maybe_unused]] const VariableList& row) const override {
        return OP()(left->evaluate(row), right->evaluate(row));
    }
    
    Cell::DataType get_type([[maybe_unused]] const VariableList& row) const override {
        return Cell::get_common_type(left->get_type(row), right->get_type(row));
    }
private:
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
};

using AdditionNode = BinaryOperationNode<std::plus<Cell>>;
using SubtractionNode = BinaryOperationNode<std::minus<Cell>>;
using MultiplicationNode = BinaryOperationNode<std::multiplies<Cell>>;
using DivisionNode = BinaryOperationNode<std::divides<Cell>>;

class VariableNode : public ExpressionNode{
public:
    VariableNode(std::string name) : name(std::move(name)) {}

    Cell evaluate(const VariableList& row) const override;
    Cell::DataType get_type(const VariableList& row) const override;

private:
    std::string name;
};

class ConstantNode : public ExpressionNode{
public:
    ConstantNode(Cell value) : value(std::move(value)) {}

    Cell evaluate([[maybe_unused]] const VariableList& _) const override {
        return value;
    }
    
    Cell::DataType get_type([[maybe_unused]] const VariableList& row) const override {
        return value.type();
    }
    
private:
    Cell value;
};


#endif