#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <shared_mutex>
#include <map>
#include <memory>
#include <valarray>
#include <functional>

#include "db/cell.h"
#include "db/expression.h"
#include "parse/token_stream.h"

using BoolVector = std::valarray<bool>;
using TableRow = std::vector<Cell>;

class Table;

/**
 * @brief Describes a column in a database table
 */
struct ColumnDescriptor{
    std::string alias;
    std::string name;
    Cell::DataType type;
    size_t index;          /**< Index of this column in rows */
    
    bool operator==(const ColumnDescriptor& other) const = default;
};

/**
 * @brief Describes columns of a table
 */
class TableHeader{
public:
    TableHeader(){}
    
    /**
     * @brief Constructor
     * @param columns Pairs (column type, column name)
     */
    TableHeader(std::vector<std::pair<Cell::DataType, std::string>> columns);
    
    /**
     * @brief Get the list of column descriptors
     */
    const std::vector<ColumnDescriptor>& get_columns() const;
    
    /**
     * @brief Get the number of columns
     */
    size_t column_count() const { return columns.size(); }
    
    /**
     * @brief Get information about a specific column
     * @return Column descriptor or `std::nullopt` if not found
     * @throws `std::runtime_error` if the name is ambiguous
     */
    std::optional<ColumnDescriptor> get_column_info(const std::string& name) const;
    
    /**
     * @brief Create a row from column name-value pairs
     */
    std::vector<Cell> create_row(const std::map<std::string, std::string>& data) const;
    
    /**
     * @brief Create a new header with an alias
     * @detail All columns in the new header are prefixed with the alias
               They can also be accessed by their original name if unique
     */
    TableHeader add_alias(const std::string& alias) const;
    
    /**
     * @brief Join two headers together
     * @return New header containing columns from both inputs
     */
    static TableHeader join(const TableHeader& left, const TableHeader& right);
    
private: 
    std::vector<ColumnDescriptor> columns;
    
    /**
     * @brief Constructor for joining two headers
     */
    TableHeader(const TableHeader& left, const TableHeader& right);
    
    /**
     * @brief Calculate the column lookup map
     * @detail Generate a map for efficiently accessing columns by name
     */
    void calculate_lookup_map();
    
    std::multimap<std::string, size_t> column_to_index;
};

class VariableList;

/**
 * @brief Represents a database table
 */
class Table{
public:
    /**
     * @brief Constructor with header
     */
    Table(TableHeader header);
    
    /**
     * @brief Constructor with column definitions
     */
    Table(std::vector<std::pair<Cell::DataType, std::string>> columns);
    
    Table(Table&& other) noexcept;
    Table& operator=(Table&& other) noexcept;
    
    /**
     * @brief Filter the table by a condition
     * @param stream The condition
     * @param variables Variable bindings for the evaluation
     * @param select_callback Callback for evaluating subqueries
     * @param negate Whether to negate the condition and filter out rows that satisfy the condition
     */
    void filter_by_condition(TokenStream& stream, const VariableList& variables,
        std::function<Table(TokenStream&, const VariableList&)> select_callback, bool negate = false);
    
    /**
     * @brief Evaluate an aggregate condition
     * @param stream The condition
     * @param variables Variable bindings for the evaluation
     * @param select_callback Callback for evaluating subqueries
     * @return True if the condition is satisfied, false otherwise
     */
    bool evaluate_aggregate_condition(TokenStream& stream, const VariableList& variables,
        std::function<Table(TokenStream&, const VariableList&)> select_callback) const;
    
    /**
     * @brief Project the table through a set of expression
     * @param expressions The projection expression
     * @param variables Variable bindings for the evaluation
     * @param aggregate_mode If true, treats expressions as aggregate functions (one row output)
     * @return New table with the projected columns
     */
    Table project(const std::vector<std::string>& expressions, const VariableList& variables, bool aggregate_mode = false) const;
    
    /**
     * @brief Remove duplicate rows from the table
     */
    void deduplicate();

    /**
     * @brief Add a row to the table with name-value pairs
     * @param values Map of column names to string values
     * @detail Ommited columns are filled with NULLs
     */
    void add_row(const std::map<std::string, std::string>& values);
    
    /**
     * @brief Add a row to the table with ordered values
     * @param data Vector of string values in column order
     */
    void add_row(const std::vector<std::string>& data);
    
    /**
     * @brief Group the table by specified columns
     * @param group_columns Column names to group by
     * @detail The rows are moved from the current table and it is left empty
     */
    std::vector<Table> group_by(const std::vector<std::string>& group_columns);
    
    /**
     * @brief Create a cross product of multiple tables
     */
    static Table cross_product(std::vector<std::pair<const Table&, std::string>> tables);
    
    /**
     * @brief Vertically join another table to this one
     */
    void vertical_join(const Table& other);
    
    /**
     * @brief Copy the table
     */
    Table clone() const;
    
    /**
     * @brief Clear the table
     */
    void clear_rows();
    
    /*
     * @brief A type for limiting method access to only some clases
     */
    class Accessor{
    private:
        Accessor() = default;
        
        friend class ConditionEvaluation;
        friend class ExpressionEvaluation;
        friend class Table;
        friend void serialize_table(const Table& table, std::ostream& os);
    };
    
    const std::vector<ColumnDescriptor>& get_columns(Accessor accessor = Accessor()) const;
    
    /**
     * @brief Evaluate an expression on the table
     */
    EvaluatedExpression evaluate_expression(TokenStream& stream, const VariableList& variables, Accessor accessor = Accessor()) const;
    
    /**
     * @brief Evaluate a condition on the table
     */
    BoolVector evaluate_condition(TokenStream& stream, const VariableList& variables,
        std::function<Table(TokenStream&, const VariableList&)> select_callback, Accessor accessor = Accessor()) const;
    
    const std::vector<TableRow>& get_rows([[maybe_unused]] Accessor accessor = Accessor()) const {return rows;};
    const TableHeader& get_header([[maybe_unused]] Accessor accessor = Accessor()) const {return header;};
private:
    TableHeader header;  
    std::vector<TableRow> rows;
    
    size_t row_count() const {
        return rows.size();
    }
    
    /**
     * @brief Add a row to the table
     */
    void add_row(TableRow data);
    
    mutable std::shared_mutex mutex;
    
};

#endif
