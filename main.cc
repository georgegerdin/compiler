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
        Parser parser(lex);
        auto ast = parser.Parse();
        boost::optional<int> i = 4;
        REQUIRE(i);
        REQUIRE(ast);
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