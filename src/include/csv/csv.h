#ifndef CSV_H
#define CSV_H

#include <exception>
#include <optional>
#include <vector>
#include <string>
#include <iostream>

using VoidableString = std::optional<std::string>;
using VoidableRow = std::vector<VoidableString>;
using VoidableTable = std::vector<VoidableRow>;

class ParsingError : std::runtime_error {
public:
    ParsingError(std::string message) : std::runtime_error(message) { }
};

VoidableTable read_csv(std::istream& input);

void write_csv(std::ostream& output, const VoidableTable& data);

#endif