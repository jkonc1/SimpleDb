#ifndef CSV_H
#define CSV_H

#include <optional>
#include <vector>
#include <string>
#include <iostream>

using VoidableString = std::optional<std::string>;
using VoidableRow = std::vector<VoidableString>;

using VoidableTable = std::vector<VoidableRow>;

/**
 * @brief Parse CSV data from a stream
 * @param input stream
 * @return Parsed table
 * @details escapes:
            - Null - `\x`
            - Comma - `\,`
            - Backslash - `\\`
 * @throws ParsingError on invalid format
 */
VoidableTable read_csv(std::istream& input);

/**
 * @brief Write CSV a stream
 * @param output stream
 * @param data table to write
 * @details escapes:
            - Null - `\x`
            - Comma - `\,`
            - Backslash - `\\`
 */
void write_csv(std::ostream& output, const VoidableTable& data);

#endif