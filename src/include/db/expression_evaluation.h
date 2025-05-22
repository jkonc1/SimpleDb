#ifndef EXPRESSION_EVALUATION_H
#define EXPRESSION_EVALUATION_H

#include "db/expression.h"
#include "db/table.h"
#include "parse/token_stream.h"
#include "db/variable_list.h"

/**
 * @brief Parses and evaluates an expressions on a table
 */
class ExpressionEvaluation {
public:
    /**
     * @brief Constructor for ExpressionEvaluation
     * @param table The table to evaluate expressions on
     * @param stream The expression
     * @param variables Variable bindings for the evaluation
     */
    ExpressionEvaluation(const Table& table, TokenStream& stream, const VariableList& variables) : 
        table(table), stream(stream), variables(variables) {}
        
    /**
     * @brief Evaluate the expression
     */
    EvaluatedExpression evaluate();
    
private:
    const Table& table;
    TokenStream& stream;      /**< Token stream containing the expression */
    const VariableList& variables; 
    
    /**
     * @brief Parse an additive expression (+ or -)
     */
    std::unique_ptr<ExpressionNode> parse_additive_expression();
    
    /**
     * @brief Parse a multiplicative expression (* or /)
     */
    std::unique_ptr<ExpressionNode> parse_multiplicative_expression();
    
    /**
     * @brief Parse a primary expression (constant, column or an aggregate)
     */
    std::unique_ptr<ExpressionNode> parse_primary_expression();
    
    /**
     * @brief Parse a COUNT expression
     */
    std::unique_ptr<ExpressionNode> parse_count();
    
    /**
     * @brief Parse an aggregate function (SUM, AVG, MIN, MAX)
     */
    std::unique_ptr<ExpressionNode> parse_aggregate();
};

#endif