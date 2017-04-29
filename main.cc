// Example program
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/hana.hpp>

#include "lexer.hh"
#include "parser.hh"


boost::optional<AstNode> Parser::ParseLetStatement() {
    LetNode node;

    //Parse 'if' identifier
    auto identifier = m_lexer.ReadToken();
    if(!identifier || identifier->data_str().compare("let") != 0) {
        Error("Parse error, expected 'let' identifier in let statement");
        return boost::none;
    }

    //Parse mut or name of variable
    auto next_token = m_lexer.PeekToken();
    if(!next_token || next_token->type() != T_NAME) {
        Error("Parse error, expected name of variable in let statement");
        return boost::none;
    }

    //Parse if variable is mutable
    if(next_token->data_str().compare("mut") == 0) {
        node.mut = true;
        m_lexer.ReadToken();
    }

    //Parse name of variable
    auto name_token = m_lexer.ReadToken();
    if(!name_token || name_token->type() != T_NAME) {
        Error("Parse error, expected name of variable in let statement");
        return boost::none;
    }

    node.var_name = name_token->data_str();

    //Parse '=' or end of let statement ';'
    next_token = m_lexer.ReadToken();
    if(!next_token) {
        Error("Parse error, end of file reached in let statement");
        return boost::none;
    }

    if(next_token->subtype() == P_ASSIGN) {    //Read assignment to variable
        auto expr = ParseExpression();
        if(!expr) {
            Error("Parse error, expected expression in let statement");
            return boost::none;
        }

        node.rhs = expr.get();
    }
    else if(next_token->subtype() != P_SEMICOLON) { //End of let statement
        Error("Parse error, expected assignment or end of let statement");
        return boost::none;
    }

    return AstNode{node};
}

boost::optional<AstNode> Parser::ParseExpression() {
    ExprNode node;

    auto first_term = ParseTerm();
    if(!first_term) {
        Error("Parse error, expected term in expression");
        return boost::none;
    }

    node.operations.push_back(first_term.get());

    bool in_expression = true;
    //Loop until there are no more add or subtract operations
    for(auto next_token = m_lexer.PeekToken();
        next_token && in_expression;
        next_token = m_lexer.PeekToken()) {

        auto type = next_token->subtype();
        if(type == P_PLUS) {
            m_lexer.ReadToken(); //Eat the plus sign
            auto term = ParseTerm(); //Read the term after the '+'
            if(!term) {
                Error("Parse error, expected a term in add expression");
                return boost::none;
            }

            //Save this add operation in the expression node
            AddNode addop;
            addop.node = term.get();
            node.operations.push_back(AstNode{addop});
        }
        else if(type == P_MINUS) {
            m_lexer.ReadToken(); //Eat the minus sign
            auto term = ParseTerm(); //Read the term after the '+'
            if(!term) {
                Error("Parse error, expected a term in add expression");
                return boost::none;
            }

            //Save this dec operation in the expression node
            DecNode decop;
            decop.node = term.get();
            node.operations.push_back(AstNode{decop});
        }
        else
            in_expression = false;
    }

    return AstNode{node};
}

//Parse an expression term
boost::optional<AstNode> Parser::ParseTerm() {
    ExprNode node;

    auto first_factor = ParseFactor();
    if(!first_factor) {
        Error("Parse error, expected a factor in expression term");
        return boost::none;
    }

    node.operations.push_back(first_factor.get());

    bool in_term = true;
    //Loop until there are no more multiply or divide operations
    for(auto next_token = m_lexer.PeekToken();
        next_token && in_term;
        next_token = m_lexer.PeekToken()) {

        auto type = next_token->subtype();
        if(type == P_MULTIPLY) {
            m_lexer.ReadToken(); //Eat the multiply sign
            auto factor = ParseFactor(); //Read the term after the '*'
            if(!factor) {
                Error("Parse error, expected a term in add expression");
                return boost::none;
            }

            //Save this multiplication operation in the expression term node
            MulNode mulop;
            mulop.node = factor.get();
            node.operations.push_back(AstNode{mulop});
        }
        else if(type == P_DIVIDE) {
            m_lexer.ReadToken(); //Eat the divide sign
            auto factor = ParseFactor(); //Read the factor after the '+'
            if(!factor) {
                Error("Parse error, expected a term in add expression");
                return boost::none;
            }

            //Save this division operation in the expression term node
            DivNode divop;
            divop.node = factor.get();
            node.operations.push_back(AstNode{divop});
        }
        else
            in_term = false;
    }
}

boost::optional<AstNode> Parser::ParseFactor() {
    auto next_token = m_lexer.PeekToken();
    if(!next_token) {
        Error("Parse error, expected a token in expression factor");
        return boost::none;
    }

    AstNode node;

    //Check if this factor is an expression enclosed in parens ( expression )
    if(next_token->subtype() == P_OPEN_PAREN) {
        m_lexer.ReadToken(); //Eat the open paren '('

        auto expr = ParseExpression();
        if(!expr) {
            Error("Parse error, expected expression");
            return boost::none;
        }

        //This factor node is an expression
        node = expr.get();

        auto closing_paren = m_lexer.ReadToken();
        if(closing_paren->subtype() != P_CLOSE_PAREN) {
            Error("Parse error, expected a paren closing expression factor");
            return boost::none;
        }
    }
    else {
        auto number = ParseNumber();
        if(!number) {
            Error("Parse error, expected a number in expression factor");
            return boost::none;
        }
        node = number.get();
    }

    return node;
}

boost::optional<AstNode> Parser::ParseNumber() {
    auto token = m_lexer.ReadToken();
    if(!token || token->type() != T_NUMBER) {
        return boost::none;
    }

    NumberNode node;
    node.value = token->data_int();
    return AstNode{node};
}

boost::optional<AstNode> Parser::ParseIdentifier() {

}


boost::optional<AstNode> Parser::ParseExpressionStatement() {
    m_lexer.ReadToken();

    return boost::none;
}

boost::optional<AstNode> Parser::ParseStatement() {
    StatementNode node;

    auto token = m_lexer.PeekToken();
    if(!token)
        return boost::none;

    auto token_type = token->type();
    switch(token_type) {
        case T_NUMBER:
        case T_STRING:
            Error("Parse error, expected statement");
            break;
        case T_NAME: {
            auto token_str = token->data_str();
            if(token_str.compare("let") == 0)
               ParseLetStatement();
            else if(token_str.compare("return") == 0)
               /*ParseReturnStatement()*/;
            else if(token_str.compare("if") == 0)
               /*ParseIfStatement()*/;
            else
               ParseExpressionStatement();
            break;
        }
        default:
            Error("Parse error, unknown statement in code block");
            return boost::none;
    }

    return AstNode{node};
}

boost::optional<AstNode> Parser::ParseStatementBlock() {
    BlockNode node;

    //Parse opening brace
    auto opening_brace = m_lexer.ReadToken();
    if(!opening_brace ||opening_brace->subtype() != P_OPEN_BRACE) {
        Error("Parse error, expected '{' parsing code block");
        return boost::none;
    }

    bool in_block = true;
    while(in_block) {
        auto next_token = m_lexer.PeekToken();

        if(!next_token) {
            Error("Parse error, unexpectedly reached end of file in code block");
            return boost::none;
        }

        if(next_token->subtype() == P_CLOSE_BRACE) {
            //Parse closing brace
            auto closing_brace = m_lexer.ReadToken();
            in_block = false;
        }
        else {
            auto statement = ParseStatement();
            if(!statement) {
                Error("Parse error, expected statement");
                return boost::none;
            }

            //Add if this is a statement node
            auto visitor = boost::hana::overload_linearly(
                [&node](StatementNode& rs) -> bool { node.statements.push_back(rs); return true; },
                [this](auto&) -> bool { Error("Parse error, expected statement node"); return false; }
                );
            if(!boost::apply_visitor(visitor, statement.get()))
                return boost::none;
        }
    }

    return AstNode{node};
}

boost::optional<AstNode> Parser::ParseFunction() {
    FunctionNode node;

    //Parse keyword "fn"
    auto identifier = m_lexer.ReadToken();
    if(!identifier || identifier->data_str().compare("fn") != 0) {
        Error("Parse error, expected keyword fn");
        return boost::none;
    }

    //Parse name of function
    auto func_name = m_lexer.ReadToken();
    if(!func_name || func_name->type() != T_NAME) {
        Error("Parse error, expected name of function");
        return boost::none;
    }

    node.name = func_name->data_str();

    //Parse open paren
    auto open_paren = m_lexer.ReadToken();
    if(!open_paren || open_paren->subtype() != P_OPEN_PAREN) {
        Error("Parse error, expected opening paren");
        return boost::none;
    }

    //Parse parametets

    //Parse close paren
    auto close_paren = m_lexer.ReadToken();
    if(!close_paren || close_paren->subtype() != P_CLOSE_PAREN) {
        Error("Parse error, expected closing paren");
        return boost::none;
    }

    //Read statement block
    auto block = ParseStatementBlock();
    if(!block) {
        Error("Parse error, expected function body");
    }

    auto visitor = boost::hana::overload_linearly(
        [](BlockNode& node) -> boost::optional<BlockNode&> { return node; },
        [](auto&) -> boost::optional<BlockNode&> { return boost::none; }
        );
    auto func_body = boost::apply_visitor(visitor, block.get());
    if(func_body)
        node.func_body = func_body.get();
    else {
        Error("Parse error, expected function body");
        return boost::none;
    }

    return AstNode{node};
}

boost::optional<AstNode> Parser::ParseModule() {
    return boost::none;
}

boost::optional<AstNode> Parser::ParseFile(const char* ) {
    FileNode file = FileNode{};
    file.modules["unnamed"] = ModuleNode{"unnnamed"};

    while(auto token = m_lexer.PeekToken()) {
        if(token->type() == T_NAME) {
            if(token->data_str() == "fn") {           //Parse a free function
                auto function = ParseFunction();

                if(function) {
                    auto& module = boost::get<ModuleNode> (file.modules["unnamed"]);
                    module.functions.push_back(function.get());
                }
            }
            else if(token->data_str() == "module") { //Parse a module
                auto ast_module = ParseModule();

                if(ast_module) {
                    auto module = boost::get<ModuleNode>(ast_module.get());
                    if(file.modules.find(module.name) != file.modules.end())
                        Error("Parse error, module already exists");
                    file.modules[module.name] = module;
                }
            }
            else
                Error("Parse error, expected function or module.");
        }
        else {
            Error("Expected identifier");
            break;
        }
    }

    return AstNode{file};
}

boost::optional<AstNode> Parser::Parse () {
    return ParseFile("empty");
}

int main() {
    std::string buffer =
        "fn main() {\n"
        "   let a = 0 + 2; \n"
        "   let b = 3\";\n"
        "   let c = 3*2;\n"
        "}";

/*    Lexer lex(buffer.c_str());


    while(auto token = lex.ReadToken()) {
        std::cout << token->type_str() << ": " << token->data_str() << "\n";
    }
*/
    std::cout << "Parsing " << buffer << "\n";
    Lexer lex2(buffer.c_str());
    Parser parser(lex2);

    auto ast = parser.Parse();
    print_ast(ast.get());
    std::cout << "Finished.";
}
