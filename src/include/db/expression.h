#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "db/cell.h"
#include <memory>

class RowReference;

class ExpressionNode{
public:
    virtual Cell evaluate(const RowReference& row) const = 0;
    
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

    Cell evaluate([[maybe_unused]] const RowReference& row) const override {
        return OP()(left->evaluate(row), right->evaluate(row));
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

    Cell evaluate(const RowReference& row) const override;

private:
    std::string name;
};

class ConstantNode : public ExpressionNode{
public:
    ConstantNode(Cell value) : value(std::move(value)) {}

    Cell evaluate([[maybe_unused]] const RowReference& _) const override {
        return value;
    }
    
private:
    Cell value;
};

#endif