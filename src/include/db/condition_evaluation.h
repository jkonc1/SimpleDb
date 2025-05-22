#ifndef CONDITION_EVALUATION_H
#define CONDITION_EVALUATION_H

#include "db/table.h"
#include "db/variable_list.h"

#include <vector>
#include <functional>

/**
 * @class ConditionEvaluation
 * @brief Evaluates SQL conditions on a table
 */
class ConditionEvaluation{
public:
    /**
     * @brief Constructor for ConditionEvaluation
     * @param table The table to evaluate conditions against
     * @param stream TokenStream containing the condition tokens
     * @param variables Variable bindings for the evaluation
     * @param select_callback Callback for evaluating subqueries
     */
    ConditionEvaluation(const Table& table, TokenStream& stream, const VariableList& variables, 
        std::function<Table(TokenStream&, const VariableList&)>& select_callback) : 
            table(table), stream(stream), variables(variables), select_callback(select_callback) {}
        
    /**
     * @brief Evaluate the condition
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate();
private:
    const Table& table;
    TokenStream& stream;
    const VariableList& variables;
    std::function<Table(TokenStream&, const VariableList&)>& select_callback;
    
    template<class T>
    using Comparator = std::function<bool(const T&, const T&)>;
        
    /**
     * @brief Evaluate a conjunctive condition (AND)
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_conjunctive_condition();
    
    /**
     * @brief Evaluate a disjunctive condition (OR)
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_disjunctive_condition();
    
    /**
     * @brief Evaluate a primary condition (one part of a disjunctive condition)
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_primary_condition();
    
    /**
     * @brief Evaluate a condition potentially preceded by NOT
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_inner_condition();
    
    /**
     * @brief Pick the correct handler for the following condition
     * @param expression Expression to evaluate the condition on
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_condition_switch(CellVector expression);
    
    /**
     * @brief Evaluate EXISTS condition
     */
    BoolVector evaluate_exists();
    
    /**
     * @brief Evaluate IS/IS NULL
     * @param expression values to check for NULL
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_is(CellVector expression);
    
    /**
     * @brief Evaluate IN condition
     * @param expression values to check for membership
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_in(CellVector expression);
    
    /**
     * @brief Evaluate BETWEEN condition
     * @param expression values to check for range
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_between(CellVector expression);
    
    /**
     * @brief Evaluate LIKE condition
     * @param expression values to check against pattern
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_like(CellVector expression);
    
    /**
     * @brief Evaluate comparison condition
     * @param expression Expression to compare
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_compare(CellVector expression);
    
    /**
     * @brief Evaluate comparison with subquery
     * @param expression Expression to compare with subquery results
     * @param comparator Comparator to use
     * @param has_any Whether ANY is present
     * @param has_all Whether ALL is present
     * @return BoolVector of the result of the condition for each row
     */
    BoolVector evaluate_compare_subquery(CellVector expression, Comparator<Cell> comparator, bool has_any, bool has_all);
    
    /**
     * @brief Convert operator token to comparison function
     * @throws InvalidQuery if the token is not a valid operator
     */
    static Comparator<Cell> operator_token_to_comparator(const Token& token);
    
    /**
     * @brief Process a SELECT statement for a single row
     * @param statement The SELECT statement
     * @param extra_vars Additional variables for the query
     * @return Result table from the SELECT
     */
    Table process_select_single_row(const std::string& statement, const BoundRow& extra_vars);
    
    /**
     * @brief Evaluate a SELECT subquery (for each row)
     * @return Vector of result tables
     */
    std::vector<Table> process_select();
    
    /**
     * @brief Evaluate a subquery for each row that only returns a single cell table
     * @return A single cell for each row
     * @throws InvalidQuery if the subquery returns more than one cell
     */
    CellVector process_select_singles();
    
    /**
     * @brief Process subqueries returning single columns
     * @return Vector of column returned from each query
     */
    std::vector<std::vector<Cell>> process_select_vectors();
    
    /**
     * @brief Extract a single cell from a table
     * @throws InvalidQuery if the table doesn't contain exactly one cell
     */
    const Cell& extract_single_cell(const Table& table);
    
    /**
     * @brief Extract a single column of a table
     * @throws InvalidQuery if the table doesn't have exactly one column
     */
    std::vector<Cell> extract_vector(const Table& table);
};

#endif