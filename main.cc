// Example program
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/hana.hpp>

#include "lexer.hh"
#include "parser.hh"
#include "symboltable.hh"
#include "sema.hh"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

std::string buffer =
        "fn func(int i, char c) -> int {\n"
        "}"
        "fn main() {\n"
        "   let a = 0; \n"
        "   let b = 2*4+3*6 + 7 / a + c(); \n"
        "}";

template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& out, AstNode const& ) {
    out << "AstNode";
    return out;
}

template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& out, Token const& ) {
    out << "Token";
    return out;
}

template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& out, std::vector<SymbolTable::Entry*> const& ) {
    out << "Entries vector";
    return out;
}

bool operator==(FileNode const& lhs, FileNode const& rhs) {
    if(lhs.name == rhs.name)
        return true;
    return false;
}

bool operator==(ModuleNode const& lhs, ModuleNode const& rhs) {
    if(lhs.name == rhs.name)
        return true;
    return false;
}

bool operator==(TypeNode const& lhs, TypeNode const& rhs) {
    if(lhs.type == rhs.type)
        return true;
    return false;
}

bool operator==(FunctionNode const& lhs, FunctionNode const& rhs) {
    if(lhs.name == rhs.name ||
       lhs.return_type == rhs.return_type)
        return true;
    return false;
}

bool operator==(BlockNode const& lhs, BlockNode const& rhs) {
    return true;
}

bool operator==(StatementNode const& lhs, StatementNode const& rhs) {
    return true;
}

bool operator==(LetNode const& lhs, LetNode const& rhs) {
    if(lhs.var_name == rhs.var_name ||
       lhs.mut == rhs.mut)
        return true;
    return false;
}

bool operator==(ExprNode const& lhs, ExprNode const& rhs) {
    return true;
}

TEST_CASE("Compiler", "[compiler]") {
    Lexer lex(buffer.c_str());

    SECTION( "lexing" ) {
        SECTION("general snippet") {
            auto lextest = [&](auto t) {
                auto result = lex.ReadToken();
                REQUIRE( result );
                CHECK( result->type() == t);
            };

            lextest(T_NAME); lextest(T_NAME); lextest(T_PUNCTUATION); lextest(T_NAME); lextest(T_NAME); lextest(T_PUNCTUATION);
            lextest(T_NAME); lextest(T_NAME); lextest(T_PUNCTUATION); lextest(T_PUNCTUATION); lextest(T_NAME); lextest(T_PUNCTUATION);
            lextest(T_PUNCTUATION); lextest(T_NAME); lextest(T_NAME); lextest(T_PUNCTUATION); lextest(T_PUNCTUATION); lextest(T_PUNCTUATION);
            lextest(T_NAME); lextest(T_NAME); lextest(T_PUNCTUATION); lextest(T_NUMBER); lextest(T_PUNCTUATION); lextest(T_NAME);
            lextest(T_NAME); lextest(T_PUNCTUATION); lextest(T_NUMBER); lextest(T_PUNCTUATION); lextest(T_NUMBER); lextest(T_PUNCTUATION);
            lextest(T_NUMBER); lextest(T_PUNCTUATION); lextest(T_NUMBER); lextest(T_PUNCTUATION); lextest(T_NUMBER); lextest(T_PUNCTUATION);
            lextest(T_NAME); lextest(T_PUNCTUATION); lextest(T_NAME); lextest(T_PUNCTUATION); lextest(T_PUNCTUATION); lextest(T_PUNCTUATION);
            lextest(T_PUNCTUATION);
            REQUIRE(!lex.ReadToken());
        }
        SECTION("individual tokens") {
            auto lextest = [] (auto snippet, auto type) {
                Lexer lex2(snippet);
                auto result = lex2.ReadToken();
                REQUIRE(result);
                CHECK(result->type() == type);
            };

            lextest("hello", T_NAME);
            lextest("\"hello\"", T_STRING);
            lextest("123", T_NUMBER);
            lextest(".", T_PUNCTUATION);
        }
    }

    SECTION( "parsing" ) {
        auto parsetest = [](auto str, std::vector<AstNode> nodes = std::vector<AstNode>{}) {
            Lexer lex(str);
            Parser parser(lex);
            auto ast = parser.Parse();
            REQUIRE(ast);

            auto it = nodes.begin();
            visit(*ast, [&](auto const& node) {
                if(it == nodes.end())
                    return;

                auto result = boost::apply_visitor(boost::hana::overload_linearly(
                    [&](decltype(node) const& check) -> bool {
                        CHECK(node == check);
                        return true;
                    },
                    [](auto const&) -> bool {
                        return false;
                    }
                ), *it);
                CHECK(result);
                ++it;
            });

            return ast;
        };

        auto parsetest_print = [&](auto str, std::vector<AstNode> nodes = std::vector<AstNode>{}) {
            auto ast = parsetest(str, nodes);
            if(ast)
                print_ast(*ast);
        };

        SECTION("general snippet") {
            Parser parser(lex);
            auto ast = parser.Parse();
            REQUIRE(ast);
            //print_ast(*ast);
        }

        SECTION("function") {
            parsetest("fn main() -> int {}",
                {
                    FileNode{},
                    ModuleNode{},
                    FunctionNode{"main", TYPE_INT},
                    BlockNode{}
                }
            );
        }
        SECTION("let statement") {
            parsetest("fn main() -> void {"
                      "  let a = 0;"
                      "}",
                {
                    FileNode{},
                    ModuleNode{},
                    FunctionNode{"main", TYPE_VOID},
                    BlockNode{},
                    StatementNode{},
                    LetNode{false, "a"},
                    ExprNode{}
                }
            );
        }
        SECTION("let addition expression") {
            parsetest("fn main() -> void {"
                      "  let a = 2+3;"
                      "}");
        }
        SECTION("let subtraction expression") {
            parsetest("fn main() -> void {"
                      "  let a = 2-3;"
                      "}");
        }
        SECTION("let multiplication expression") {
            parsetest("fn main() -> void {"
                      "  let a = 2*3;"
                      "}");
        }
        SECTION("let divide expression") {
            parsetest("fn main() -> void {"
                      "  let a = 2/3;"
                      "}");
        }
    }
    SECTION("symbol table") {
        SECTION("general snippet") {
            Parser parser(lex);
            auto ast = parser.Parse().get();
            SymbolTable sym(ast);
            sym.Generate();
            //print_symbol_table(sym);
            REQUIRE(sym.Lookup("a"));
            REQUIRE(sym.Lookup("b"));
            REQUIRE(sym.Lookup("c"));
            REQUIRE(sym.Lookup("func"));
            REQUIRE(sym.Lookup("i"));
            REQUIRE(sym.Lookup("main"));
        }
    }
    SECTION("semantic analysis") {
        SECTION("use local variable") {
            Lexer lex("fn main() -> void {"
                      "  let a = 2+3;"
                      "  let b = a+2;"
                      "}");
            Parser parser(lex);
            auto ast = parser.Parse().get();
            SymbolTable sym(ast);
            sym.Generate();
            REQUIRE(sym.Lookup("main"));
            REQUIRE(sym.Lookup("a"));
            REQUIRE(sym.Lookup("b"));

            Sema sa(ast, sym);
            REQUIRE(sa.Analyse());
        }
        SECTION("reserved keyword") {
            Lexer lex("fn main() -> void {"
                      "  let int = 2+3;"
                      "}");
            Parser parser(lex);
            auto ast = parser.Parse().get();
            SymbolTable sym(ast);
            sym.Generate();
            REQUIRE(sym.Lookup("main"));

            Sema sa(ast, sym);
            REQUIRE(!sa.Analyse());
        }
    }

}

#if 0
int main() {
    

/*    Lexer lex(buffer.c_str());


    while(auto token = lex.ReadToken()) {
        std::cout << token->type_str() << ": " << token->data_str() << "\n";
    }
*/
    std::cout << "Parsing " << buffer << "\n";
    Lexer lex2(buffer.c_str());
    Parser parser(lex2);

    auto ast = parser.Parse().get();
//    print_ast(ast);
    
    SymbolTable sym(ast);
    sym.Generate();
//    print_symbol_table(sym);  
}
#endif
