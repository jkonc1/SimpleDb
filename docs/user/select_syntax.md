<search_condition> ::= 
      [NOT] <expression> BETWEEN <expression> AND <expression>
    | [NOT] <expression> LIKE <string>
    | <column> IS [NOT] NULL
    | <expression> (= | > | < | >= | <= | <>) [ANY | ALL] (<unordered_query>)
    | <expression> (= | > | < | >= | <= | <>) <expression>
    | <expression> [NOT] IN (<value_list> | <unordered_query>)
    | EXISTS (<unordered_query>)
    | <search_condition> AND <search_condition>
    | <search_condition> OR <search_condition>
    | NOT <search_condition>

<expression> ::= 
      <column>
    | <value>
    | COUNT([ALL | DISTINCT] <column>)
    | MAX(<expression>)
    | MIN(<expression>)
    | SUM([DISTINCT] <expression>)
    | AVG([DISTINCT] <expression>)
    | <expression> + <expression>
    | <expression> - <expression>
    | <expression> * <expression>
    | <expression> / <expression>

<select_statement> ::= 
    SELECT [ALL | DISTINCT] (<expression> | *) {, <expression>}*
    FROM <table> [<correlation_var>] {, <table> [<correlation_var>]}*
    [WHERE <search_condition>]
    [GROUP BY <column> {, <column>}* [HAVING <search_condition>]]

