#ifndef EXPRESSION_EVALUATION_H
#define EXPRESSION_EVALUATION_H

#include "db/expression.h"
#include "db/table.h"
#include "parse/token_stream.h"
#include "db/variable_list.h"

class ExpressionEvaluation {
public:

    ExpressionEvaluation(const Table& table, TokenStream& stream, const VariableList& variables) : 
        table(table), stream(stream), variables(variables) {}
        
    EvaluatedExpression evaluate();
    
private:
    const Table& table;
    TokenStream& stream;
    const VariableList& variables;
    
    std::unique_ptr<ExpressionNode> parse_additive_expression();
    std::unique_ptr<ExpressionNode> parse_multiplicative_expression();
    std::unique_ptr<ExpressionNode> parse_primary_expression();
    std::unique_ptr<ExpressionNode> parse_count();
    std::unique_ptr<ExpressionNode> parse_aggregate();
};

#endif