#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "db/cell.h"

#include <valarray>
#include <functional>
#include <memory>

using CellVector = std::valarray<Cell>;

/**
 * @brief Result of evaluating an expression on a table
 */
struct EvaluatedExpression {
    Cell::DataType type;
    CellVector values;
};

class VariableList;

/**
 * @brief Abstract base class for expression tree nodes
 */
class ExpressionNode{
public:
    /**
     * @brief Evaluate the expression for a given row
     */
    virtual Cell evaluate(const VariableList& row) const = 0;
    
    /**
     * @brief Get the data type of the expression result
     */
    virtual Cell::DataType get_type(const VariableList& row) const = 0;
    
    ExpressionNode() = default;
    
    ExpressionNode(ExpressionNode&&) = default;
    
    ExpressionNode(const ExpressionNode&) = delete;
    
    ExpressionNode& operator=(ExpressionNode&&) = default;
    
    ExpressionNode& operator=(const ExpressionNode&) = delete;
    
    virtual ~ExpressionNode() = default;
};

/**
 * @brief Node representing a binary operation
 * @tparam OP Functor type for the binary operation
 */
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
    std::unique_ptr<ExpressionNode> left;  /**< Left operand expression */
    std::unique_ptr<ExpressionNode> right; /**< Right operand expression */
};

using AdditionNode = BinaryOperationNode<std::plus<Cell>>;

using SubtractionNode = BinaryOperationNode<std::minus<Cell>>;

using MultiplicationNode = BinaryOperationNode<std::multiplies<Cell>>;

using DivisionNode = BinaryOperationNode<std::divides<Cell>>;

/**
 * @brief Node representing a variable
 */
class VariableNode : public ExpressionNode{
public:
    /**
     * @param name Name of the variable to reference
     */
    VariableNode(std::string name) : name(std::move(name)) {}

    Cell evaluate(const VariableList& row) const override;
    
    Cell::DataType get_type(const VariableList& row) const override;

private:
    std::string name;
};

/**
 * @brief Node representing a constant value
 */
class ConstantNode : public ExpressionNode{
public:
    /**
     * @param value Constant value to store
     */
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