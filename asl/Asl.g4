//////////////////////////////////////////////////////////////////////
//
//    Asl - Another simple language (grammar)
//
//    Copyright (C) 2020-2030  Universitat Politecnica de Catalunya
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public License
//    as published by the Free Software Foundation; either version 3
//    of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    contact: José Miguel Rivero (rivero@cs.upc.edu)
//             Computer Science Department
//             Universitat Politecnica de Catalunya
//             despatx Omega.110 - Campus Nord UPC
//             08034 Barcelona.  SPAIN
//
//////////////////////////////////////////////////////////////////////

grammar Asl;

//////////////////////////////////////////////////
/// Parser Rules
//////////////////////////////////////////////////

// A program is a list of functions
program : function+ EOF
        ;

// A function has a name, a list of parameters and a list of statements
function
        : FUNC ID '(' parameter_decl ')' (':' type)? declarations statements ENDFUNC
        ;

declarations
        : (variable_decl)*
        ;

parameter_decl
        : (ID ':' type) (',' ID ':' type)*
        |  
        ;

variable_decl
        : VAR ID (',' ID)* ':' type
        ;

type    : basic_type
        | array_type
        ;

basic_type    
        : INT
        | FLOAT
        | CHAR
        | BOOL
        ;

array_type
        : ARRAY '[' INTVAL ']' 'of' basic_type
        ;

statements
        : (statement)*
        ;

call    : ident '(' expr? (',' expr)* ')'
        ;

// The different types of instructions
statement
          // Assignment
        : left_expr ASSIGN expr ';'           # assignStmt
          // if-then-else statement (else is optional)
        | IF expr THEN statements (ELSE statements)? ENDIF       # ifStmt
          // while loop
        | WHILE expr DO statements ENDWHILE      # whileStmt
          // A function/procedure call has a list of arguments in parenthesis (possibly empty) 
          // This ignores return value from function (void methods can only be called this way,
          // while non-void methods can be called both as an expression or as a statement).
        | call ';'                              # procCall
          // Read a variable
        | READ left_expr ';'                    # readStmt
          // Write an expression
        | WRITE expr ';'                        # writeExpr
          // Write a string
        | WRITE STRING ';'                      # writeString
          //return value
        | RETURN (expr)? ';'                    # returnStmt
        ;

// Grammar for left expressions (l-values in C++)
left_expr
        : ident                                         # exprIdent
        | ident '[' expr ']'                            # arrLeftExpr                           
        ;

// Grammar for expressions with boolean, relational and aritmetic operators
          // A function that returns a value (non-void methods only).
expr    : left_expr                                     # leftExpr
        | call                                          # funcCall
        | op=(NOT|PLUS|MINUS) expr                      # unary
        | expr op=(MUL|DIV|MOD) expr                    # arithmetic
        | expr op=(PLUS|MINUS) expr                     # arithmetic
        | expr op=(EQUAL|NEQ|GT|LT|GE|LE) expr          # relational
        | expr op=AND expr                              # logical
        | expr op=OR expr                               # logical
        | INTVAL                                        # value
        | FLOATVAL                                      # value
        | CHARVAL                                       # value
        | BOOLVAL                                       # value
        | '(' expr ')'                                  # parenthesis
        ;

// Identifiers
ident   : ID
        ;

//////////////////////////////////////////////////
/// Lexer Rules
//////////////////////////////////////////////////

/*------Assignment Operators------*/
ASSIGN    : '=' ;

/*------Comparison Operators------*/
EQUAL     : '==' ;
NEQ       : '!=';
GT        : '>';
GE        : '>=';
LT        : '<';
LE        : '<=';

/*------Arithmetic Operators------*/
PLUS      : '+' ;
MINUS     : '-';
MUL       : '*';
DIV       : '/';
MOD       : '%';

/*-------Logical Operators--------*/
NOT       : 'not';
AND       : 'and';
OR        : 'or';

/*--Variable declaration & Types--*/
VAR       : 'var';
INT       : 'int';
FLOAT     : 'float';
CHAR      : 'char';
BOOL      : 'bool';
ARRAY     : 'array';

/*-----Conditional Statements-----*/
IF        : 'if' ;
THEN      : 'then' ;
ELSE      : 'else' ;
ENDIF     : 'endif' ;

/*--------Loop Statements---------*/
WHILE     : 'while' ;
DO        : 'do';
ENDWHILE  : 'endwhile';

/*-Function declaration & Return--*/
FUNC      : 'func' ;
ENDFUNC   : 'endfunc' ;
RETURN    : 'return';

/*--------------I/O---------------*/
READ      : 'read' ;
WRITE     : 'write' ;

/*----------Type values-----------*/
INTVAL    : ('0'..'9')+ ;
FLOATVAL  : ('0'..'9')+ '.' ('0'..'9')+ ;
CHARVAL   : '\'' (ESC_SEQ | ~('\\'|'\'')) '\'' ;
BOOLVAL   : ('true' | 'false');

/*----------Identifiers-----------*/
ID        : ('a'..'z'|'A'..'Z') ('a'..'z'|'A'..'Z'|'_'|'0'..'9')* ;

// Strings (in quotes) with escape sequences
STRING    : '"' ( ESC_SEQ | ~('\\'|'"') )* '"' ;

fragment
ESC_SEQ   : '\\' ('b'|'t'|'n'|'f'|'r'|'"'|'\''|'\\') ;

// Comments (inline C++-style)
COMMENT   : '//' ~('\n'|'\r')* '\r'? '\n' -> skip ;

// White spaces
WS        : (' '|'\t'|'\r'|'\n')+ -> skip ;
// Alternative description
// WS        : [ \t\r\n]+ -> skip ;
