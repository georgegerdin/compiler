#ifndef __lexer_h__
#define __lexer_h__

struct Punctuation {
    const char* chars;
    int id;
};

enum PunctuationTypes : int {
    P_NIL = 0,
    P_LOGIC_EQUAL,
    P_LOGIC_AND,
    P_RIGHT_ARROW,
    P_ASSIGN,
    P_PLUS,
    P_MINUS,
    P_MULTIPLY,
    P_DIVIDE,
    P_SEMICOLON,
    P_DOT,
    P_COMMA,
    P_OPEN_PAREN,
    P_CLOSE_PAREN,
    P_OPEN_BRACE,
    P_CLOSE_BRACE
};

Punctuation punctuations[] = {
    {"==", P_LOGIC_EQUAL},
    {"&&", P_LOGIC_AND},
    {"->", P_RIGHT_ARROW},
    {"=", P_ASSIGN},
    {"+", P_PLUS},
    {"-", P_MINUS},
    {"*", P_MULTIPLY},
    {"/", P_DIVIDE},
    {";", P_SEMICOLON},
    {".", P_DOT},
    {",", P_COMMA},
    {"(", P_OPEN_PAREN},
    {")", P_CLOSE_PAREN},
    {"{", P_OPEN_BRACE},
    {"}", P_CLOSE_BRACE},

    {nullptr, P_NIL}
};

enum TokenTypes : int {
    T_STRING = 0,
    T_LITERAL,
    T_NAME,
    T_NUMBER,
    T_PUNCTUATION
};

struct StringToken {
    StringToken(std::string idata) : data(idata) { }
    std::string data;
};

struct LiteralToken {
    LiteralToken(std::string idata) : data(idata) { }
    std::string data;
};

struct NameToken {
    NameToken(std::string idata) : data(idata) { }
    std::string data;
};

struct NumberToken {
    NumberToken(int idata) : data(idata) { }
    int data;
};

struct PunctuationToken {
    PunctuationToken(const char* idata, int i) : data(idata), id(i) { }
    std::string data;
    int id;
};

std::string type_as_str(const StringToken) {return "STRING";}
std::string type_as_str(const LiteralToken) {return "LITERAL";}
std::string type_as_str(const NumberToken) {return "NUMBER";}
std::string type_as_str(const NameToken) {return "NAME";}
std::string type_as_str(const PunctuationToken) {return "PUNCTUATION";}

int type_as_int(const StringToken) {return T_STRING;}
int type_as_int(const LiteralToken) {return T_LITERAL;}
int type_as_int(const NumberToken) {return T_NUMBER;}
int type_as_int(const NameToken) {return T_NAME;}
int type_as_int(const PunctuationToken) {return T_PUNCTUATION;}

std::string data_as_str(const StringToken tok) {return tok.data;}
std::string data_as_str(const LiteralToken tok) {return tok.data;}
std::string data_as_str(const NumberToken tok) {return std::to_string(tok.data);}
std::string data_as_str(const NameToken tok) {return tok.data;}
std::string data_as_str(const PunctuationToken tok) {return tok.data;}

typedef boost::variant<StringToken, LiteralToken, NumberToken, NameToken, PunctuationToken> TokenData;

struct Token {
    int line;
    TokenData data;

    Token(int iline, TokenData idata) : line(iline), data(idata) { }

    std::string type_str() {
        auto my_visitor = boost::hana::overload([](auto token) -> std::string { return type_as_str(token); });
        return boost::apply_visitor(my_visitor, data);
    }

    int type() {
        auto my_visitor = boost::hana::overload([](auto token) -> int { return type_as_int(token); });
        return boost::apply_visitor(my_visitor, data);
    }

    int subtype() {
        auto my_visitor = boost::hana::overload_linearly(
            [](PunctuationToken& token) -> int { return token.id; },
            [](auto&) -> int {return P_NIL;}
            );
        return boost::apply_visitor(my_visitor, data);
    }

    std::string data_str() {
       auto my_visitor = boost::hana::overload([] (auto token) -> std::string {return data_as_str(token);});
       return boost::apply_visitor(my_visitor, data);
    }

    int data_int() {
       auto my_visitor = boost::hana::overload_linearly(
                   [] (NumberToken& token) -> int {return token.data;},
                   [] (auto& ) -> int {return 0;}
                );
       return boost::apply_visitor(my_visitor, data);
    }
};

class Lexer {
public:
    Lexer(const char* buffer, int start_line = 0);

    boost::optional<Token> ReadToken();
    boost::optional<Token> PeekToken();
    bool ReadWhitespace();
    boost::optional<Token> ReadString();
    boost::optional<Token> ReadCharacter();
    boost::optional<Token> ReadNumber();
    boost::optional<Token> ReadName();
    boost::optional<Token> ReadPunctuation();

    void Error(const char* error_string) {m_error = error_string; m_error_line = m_line; std::cout << error_string << std::endl; }
protected:
    const char* m_buffer;
    const char* m_current;
    int m_line;

    std::string m_error;
    int m_error_line;
    boost::optional<Token> m_peek = boost::none;
};

Lexer::Lexer(const char* buffer, int start_line) : m_buffer(buffer), m_current(buffer), m_line(start_line) { }

boost::optional<Token> Lexer::ReadToken() {
    if(m_peek) {
        auto ret = m_peek;
        m_peek = boost::none;
        return ret;
    }

    if(!ReadWhitespace())
        return boost::none;

    boost::optional<Token> result = boost::none;

    auto c = *m_current;
    if(c == '\"')
        result = ReadString();
    else if(c == '\'')
        result =ReadCharacter();
    else if(c >= '0' && c <= '9')
        result = ReadNumber();
    else if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_' )
        result = ReadName();
    else {
        result = ReadPunctuation();
        if(!result)
            Error("Unknown punctuation");
    }

    return result;
}

boost::optional<Token> Lexer::PeekToken() {
    if(m_peek)
        return m_peek;
    m_peek = ReadToken();
    return m_peek;
}

bool Lexer::ReadWhitespace() {
    bool in_whitespace = true;
    while(in_whitespace) {
        if(*m_current == ' ' || *m_current == '\t')
            m_current++;
        //Skip comments
        else if(*m_current == '/') {
            if(*(m_current+1) == '/') { // skip double-slash // comments
                m_current++;
                do {
                    m_current++;
                    if(*m_current == '\0')
                        return false; //Reached end of stream inside comment
                } while(*m_current != '\n');

                m_line++;
            }
            if(*(m_current+1) == '*') { // skip c-style block comments /* ... */
                m_current++;

                bool in_comment = true;
                while(in_comment) {
                    m_current++;
                    if(*m_current == '\0')
                        return false; //Reached end of stream inside block comment
                    if(*m_current == '\n')
                        m_line++;
                    else if(*m_current == '/') {
                        if(*(m_current-1) == '*')
                            in_comment = false;
                    }
                }
				m_current++;
            }
            else
                in_whitespace = false;
        }
        else if(*m_current == '\n') {
            m_line++;
            m_current++;
        }
        else {
            if(*m_current == '\0')
                return false;

            in_whitespace = false;
        }
    }

    return true;
}

boost::optional<Token> Lexer::ReadString() {
    bool in_string = true;
    std::string value = "";

    //leading quote
    m_current++;

    boost::optional<Token> result = boost::none;
    while(in_string ) {
        char c = *m_current;

        if(c == '\"') { //Reached end of string
            in_string = false;
            result = Token { m_line, StringToken(value) };
            m_current++;
        }
        else if(c == '\0') {
            in_string = false;
            Error("End of file in string literal.");
        }
        else if(c == '\n') {
            in_string = false;
            Error("End of line in string literal.");
        }
        else {
            value+= c;
            m_current++;
        }
    }

    return result;
}

boost::optional<Token> Lexer::ReadCharacter() {
    m_current++;

    while(*m_current != '\'')
        m_current ++;

    return boost::none;
}

boost::optional<Token> Lexer::ReadNumber() {
    bool in_number = true;

    std::string value = "";
    value+= *m_current;
    m_current++;

    boost::optional<Token> result = boost::none;

    while(in_number) {
        char c = *m_current;

        if(c >= '0' && c <= '9') {
            value+= *m_current;
            m_current++;
        }
        else {
            in_number = false;
            result = Token { m_line, NumberToken(std::stoi(value, nullptr, 10)) };
        }
    }

    return result;
}

boost::optional<Token> Lexer::ReadName() {
    bool in_name = true;

    std::string value = "";
    value+= *m_current;

    m_current++;

    boost::optional<Token> result = boost::none;

    while(in_name) {
        char c= *m_current;

        if(( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_') {
            value+= c;
            m_current++;
        }
        else {
            in_name = false;
            result = Token { m_line, NameToken(value) };
        }
    }
    return result;
}

boost::optional<Token> Lexer::ReadPunctuation() {
    boost::optional<Token> result = boost::none;
    
    //Check against all punctuations, longest first
    bool match = false;
    for(int i = 0; punctuations[i].chars && !match; ++i) {
        int len = strlen(punctuations[i].chars);

        match = true;
        for(int j = 0; j < len && match; ++j) {
            if(m_current[j] == '\0')
                match = false;
            else if(m_current[j] != punctuations[i].chars[j])
                match = false;
        }

        if(match) {
            result = Token {m_line, PunctuationToken(punctuations[i].chars, punctuations[i].id)};
            m_current+= len;
        }
    }

    if(!result)
        m_current++;

    return result;
}

#endif //__lexer_h__