#ifndef __parser_h__
#define __parser_h__

struct FileNode;
struct ModuleNode;
struct FunctionNode;
struct BlockNode;
struct StatementNode;
struct EmptyStatementNode;
struct LetNode;
struct TypeNode;
struct ExprNode;
struct AddNode;
struct DecNode;
struct MulNode;
struct DivNode;
struct AssignNode;
struct LogicAndNode;
struct NumberNode;
struct StringNode;
struct IdentifierNode;
struct FnCallNode;
struct ParameterNode;

enum SimpleType {
    TYPE_INT=1,
    TYPE_UINT,
    TYPE_CHAR,
    TYPE_VOID
};

struct NamedType {
    std::string name;
};

bool operator==(NamedType const& lhs, NamedType const& rhs) {
    if(lhs.name == rhs.name)
        return true;
    return false;
}

typedef boost::variant< boost::recursive_wrapper<FileNode>,
                        boost::recursive_wrapper<ModuleNode>,
                        boost::recursive_wrapper<FunctionNode>,
                        boost::recursive_wrapper<BlockNode>,
                        boost::recursive_wrapper<StatementNode>,
                        boost::recursive_wrapper<EmptyStatementNode>,
                        boost::recursive_wrapper<LetNode>,
                        boost::recursive_wrapper<TypeNode>,
                        boost::recursive_wrapper<ExprNode>,
                        boost::recursive_wrapper<AddNode>,
                        boost::recursive_wrapper<DecNode>,
                        boost::recursive_wrapper<MulNode>,
                        boost::recursive_wrapper<DivNode>,

                        boost::recursive_wrapper<AssignNode>,
                        boost::recursive_wrapper<LogicAndNode>,
                        boost::recursive_wrapper<NumberNode>,
                        boost::recursive_wrapper<StringNode>,
    
                        boost::recursive_wrapper<IdentifierNode>,
                        boost::recursive_wrapper<FnCallNode>,
                        boost::recursive_wrapper<ParameterNode>
                        > AstNode;

using std::vector;
using std::map;
struct FileNode { std::string name; vector<ModuleNode> modules; };
struct ModuleNode { boost::optional<std::string> name; vector<AstNode> functions;
                    ModuleNode(std::string iname) : name(iname) {} ModuleNode() { } };
struct TypeNode { boost::variant<SimpleType, NamedType> type;
                  TypeNode() {}
                  TypeNode(boost::variant<SimpleType, NamedType> itype) : type(itype) {}};
struct BlockNode { vector<StatementNode> statements; };
struct StatementNode { AstNode expr; };
struct EmptyStatementNode { };
struct FunctionNode { std::string name; TypeNode return_type;
                      vector<ParameterNode> parameters;
                      BlockNode func_body;
                      FunctionNode() {}
                      FunctionNode(const char* iname, boost::variant<SimpleType, NamedType> itype)
                        : name(iname), return_type(itype) {} };
struct ExprNode { vector<AstNode> operations; };
struct AddNode { AstNode node; };
struct DecNode { AstNode node; };
struct MulNode { AstNode node; };
struct DivNode { AstNode node; };
struct LetNode { bool mut = false; std::string var_name; AstNode rhs; };
struct AssignNode { AstNode lhs; AstNode rhs; };
struct LogicAndNode { AstNode lhs; AstNode rhs; };
struct NumberNode { int value; };
struct StringNode { std::string value; };
struct IdentifierNode { std::string identifier; };
struct FnCallNode { std::string identifier; };
struct ParameterNode { TypeNode type; std::string name; };

class Parser {
public:
    Parser(Lexer& lex) : m_lexer(lex) { }

    boost::optional<AstNode> Parse();
    boost::optional<AstNode> ParseFile(const char* filename);
    boost::optional<AstNode> ParseFunction();
    boost::optional<AstNode> ParseModule();
    boost::optional<AstNode> ParseStatementBlock();
    boost::optional<AstNode> ParseStatement();
    boost::optional<AstNode> ParseLetStatement();
    boost::optional<TypeNode> ParseType();
    boost::optional<vector<ParameterNode>> ParseParameters();
    boost::optional<AstNode> ParseExpression();
    boost::optional<AstNode> ParseExpressionStatement();
    boost::optional<AstNode> ParseTerm();
    boost::optional<AstNode> ParseFactor();
    boost::optional<AstNode> ParseNumber();
    boost::optional<AstNode> ParseIdentifier();

    void Error(std::string error_str) { std::cout << error_str << std::endl; };
protected:
    Lexer& m_lexer;
};

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
        
        //Read the ending semi-colon
        next_token = m_lexer.ReadToken(); 
    }
    
    if(next_token->subtype() != P_SEMICOLON) { //End of let statement
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
    
    return AstNode{node};
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
    else if(next_token->type() == T_NAME) {
        auto ident = ParseIdentifier();
        if(!ident) {
            Error("Parse error, expected identifier in expression factor");
            return boost::none;
        }
        
        node = ident.get();
    }
    else
    {
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
    auto identifier_name = m_lexer.ReadToken();
    if(!identifier_name || identifier_name->type() != T_NAME) {
        Error("Parse error, expected identifier");
        return boost::none;
    }
    
    auto next_token = m_lexer.PeekToken();
    if(next_token && next_token->subtype() == P_OPEN_PAREN) {
        m_lexer.ReadToken(); //Eat open paren
        
        //Parse arguments
        
        auto closing_paren = m_lexer.ReadToken();
        if(!closing_paren || closing_paren->subtype() != P_CLOSE_PAREN) {
            Error("Parse error, function call arguments must end with ')'");
            return boost::none;
        }
        
        auto fn_call_node = FnCallNode{identifier_name.get().data_str()};
        
        return AstNode{fn_call_node}; 
    }
    
    return AstNode{IdentifierNode{identifier_name.get().data_str()}};
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
            if(token_str.compare("let") == 0) {
                auto let_statement = ParseLetStatement();   
                if(!let_statement)
                    return boost::none;
                
                node.expr = *let_statement;
            }
            else if(token_str.compare("return") == 0)
               /*ParseReturnStatement()*/;
            else if(token_str.compare("if") == 0)
               /*ParseIfStatement()*/;
            else
               ParseExpressionStatement();
            break;
        }
        case T_PUNCTUATION: {
                if(token->subtype() == P_SEMICOLON) {
                    //Empty statement
                    m_lexer.ReadToken();
                    return AstNode{EmptyStatementNode{}};
                }
            }
            break;
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
                return boost::none;
            }

            //Add if this is a statement node
            auto visitor = boost::hana::overload_linearly(
                [&node](StatementNode& rs) -> bool { node.statements.push_back(rs); return true; },
                [](EmptyStatementNode&) -> bool { return true; }, //Ignore empty statements
                [this](auto&) -> bool { Error("Parse error, expected statement node"); return false; }
                );
            if(!boost::apply_visitor(visitor, statement.get()))
                return boost::none;
        }
    }

    return AstNode{node};
}

boost::optional<TypeNode> Parser::ParseType() {
    auto type = m_lexer.ReadToken();
    if(!type || type->type() != T_NAME) {
        Error("Parse error, expected type");
        return boost::none;
    }
    
    TypeNode node;
        
    auto str = type->data_str();
    if(str.compare("int") == 0)
        node.type = TYPE_INT;
    else if(str.compare("uint") == 0)
        node.type = TYPE_UINT;
    else if(str.compare("char") == 0)
        node.type = TYPE_CHAR;
    else if(str.compare("void") == 0)
        node.type = TYPE_VOID;
    else {
        node.type = NamedType{str};
    }
    
    return node;
}

boost::optional<vector<ParameterNode>> Parser::ParseParameters() {
    vector<ParameterNode> params;

    bool in_parameter_list = true;
    while(in_parameter_list) {
        auto next_token = m_lexer.PeekToken();
        if(!next_token) {
            Error("Parse error, Unexpected end of file in parameter list");
            return boost::none;
        }
        
        if(next_token->subtype() == P_CLOSE_PAREN) {
            in_parameter_list = false;
        }
        else {
            ParameterNode node;
            auto type = ParseType();
            if(!type) {
                Error("Parse error, expected type of parameter in parameter list");
                return boost::none;
            }
            
            node.type = type.get();
            
            next_token = m_lexer.PeekToken();
            if(!next_token) {
                Error("Parse error, Unexpected end of file in parameter list");
                return boost::none;
            }
            
            if(next_token->type() == T_NAME) {
                auto name = m_lexer.ReadToken();
                if(!name || name->type() != T_NAME) {
                    Error("Parse error, expected name of parameter");
                    return boost::none;
                }
                node.name = name->data_str();
                
                next_token = m_lexer.PeekToken();
                if(!next_token) {
                    Error("Parse error, Unexpected end of file in parameter list");
                    return boost::none;
                }
            }
            
            if(next_token->subtype() == P_COMMA)
                m_lexer.ReadToken(); //Eat the comma
            else {
                if(next_token->subtype() == P_CLOSE_PAREN)
                    in_parameter_list = false;
                else {
                    Error("Parse error, invalid token in parameter list");
                    return boost::none;
                }
            }
            params.emplace_back(node);
        }
    }
    
    return params;
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

    //Parse parameters
    auto parameters = ParseParameters();
    if(!parameters) {
        Error("Parse error, expected function parameters");
        return boost::none;
    }
    
    node.parameters = parameters.get();

    //Parse close paren
    auto close_paren = m_lexer.ReadToken();
    if(!close_paren || close_paren->subtype() != P_CLOSE_PAREN) {
        Error("Parse error, expected closing paren");
        return boost::none;
    }
    
    //Parse optional return type
    auto next_token = m_lexer.PeekToken();
    if(next_token && next_token->subtype() == P_RIGHT_ARROW) {
        m_lexer.ReadToken(); //Eat '->' operator
        
        auto type = ParseType();
        if(!type) {
            Error("Parse error, expected type expression");
            return boost::none;
        }
        
        node.return_type = type.get();
    }
    else {
        node.return_type = TypeNode{TYPE_VOID};
    }

    //Read statement block
    auto block = ParseStatementBlock();
    if(!block) {
        return boost::none;
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

    auto module = ModuleNode{};

    while(auto token = m_lexer.PeekToken()) {
        if(token->type() == T_NAME) {
            if(token->data_str() == "fn") {           //Parse a free function
                auto function = ParseFunction();

                if(!function)
                    return boost::none;

                module.functions.push_back(function.get());
            }
            else if(token->data_str() == "module") { //Parse a module
                auto ast_module = ParseModule();

                if(!ast_module)
                    return boost::none;

                auto m = boost::get<ModuleNode>(ast_module.get());
                for(auto const& it : file.modules) {
                    if(it.name && m.name) {
                        if (it.name.get() == m.name.get())
                            Error("Parse error, module already exists");
                    }
                }
                file.modules.push_back(m);
            }
            else {
                Error("Parse error, expected function or module.");
                return boost::none;
            }
        }
        else {
            Error("Expected identifier");
            return boost::none;
        }
    }

    file.modules.emplace_back(module);

    return AstNode{file};
}

boost::optional<AstNode> Parser::Parse () {
    return ParseFile("empty");
}

void tab(int depth) {
    for(int i = 0; i < depth; ++i)
        std::cout << "..";
}

void print_node(FileNode const& node, int depth);
void print_node(ModuleNode const& node, int depth);
void print_node(FunctionNode const& node, int depth);
void print_node(BlockNode const& node, int depth);
void print_node(StatementNode const& node, int depth);
void print_node(LetNode const& node, int depth);
void print_node(TypeNode const& node, int depth);
void print_node(AssignNode const& node, int depth) ;
void print_node(LogicAndNode const& node, int depth);
void print_node(NumberNode const& node, int depth);
void print_node(StringNode const& node, int depth);
void print_node(ParameterNode const& node, int depth);

struct print_node_visitor : public boost::static_visitor<> {
    print_node_visitor(int d) : m_depth(d) {}
    template <typename T>
    void operator()(T node) const { tab(m_depth); print_node(node, m_depth); }
    int m_depth;
};

void print_node(FileNode const& node, int depth) {
    std::cout << "File node\n";

    for(auto& child : node.modules)
        print_node(child, depth+1);
}

void print_node(ModuleNode const& node, int depth) {
    if(node.name)
        std::cout << "Module node: " << node.name.get() << "\n";
    else
        std::cout << "Module node (global) \n";

    for(auto& children : node.functions)
        boost::apply_visitor(print_node_visitor{depth+1}, children);
}

void print_node(FunctionNode const& node, int depth) {
    std::cout << "Function node: " << node.name << " return type ";
    print_node(node.return_type, depth);
    std::cout << "\n";
    for(auto& p : node.parameters) {
        tab(depth); print_node(p, depth); 
    }
    print_node(node.func_body, depth+1);
}

void print_node(BlockNode const& node, int depth) {
    tab(depth); std::cout << "Block node: \n";
    for(auto& statement : node.statements) {
        tab(depth+1); print_node(statement, depth+1);
    }
}

void print_node(StatementNode const& node, int depth) {
    std::cout << "Statement\n";
    boost::apply_visitor(print_node_visitor{depth+1}, node.expr);
}

void print_node(EmptyStatementNode const&, int) {
    std::cout << "Empty statement\n";
}

void print_node(TypeNode const& node, int) {
    boost::apply_visitor(boost::hana::overload(
        [](SimpleType const& type) {
            switch(type) {
            case TYPE_INT:
                std::cout << "int"; break;
            case TYPE_UINT:
                std::cout << "uint"; break;
            case TYPE_CHAR:
                std::cout << "char"; break;
            case TYPE_VOID:
                std::cout << "void"; break;
            default:
                break;
            };
        },
        [](NamedType const& type) {
            std::cout << type.name;
        }
    ), node.type);
}

void print_node(LetNode const& node, int depth) {
    std::cout << "Let node " << node.var_name << "\n";
    boost::apply_visitor(print_node_visitor{depth+1}, node.rhs);
}

void print_node(ExprNode const& node, int depth) {
    std::cout << "Expression\n";
    for(auto& op : node.operations) {
        boost::apply_visitor(print_node_visitor{depth+1}, op);
    }
}

void print_node(AddNode const& node, int depth) {
    std::cout << "AddNode\n";
    boost::apply_visitor(print_node_visitor{depth+1}, node.node);;
}

void print_node(DecNode  const& node, int depth) {
    std::cout << "DecNode\n";
    boost::apply_visitor(print_node_visitor{depth+1}, node.node);
}

void print_node(MulNode  const& node, int depth) {
    std::cout << "MulNode\n";
    boost::apply_visitor(print_node_visitor{depth+1}, node.node);
}

void print_node(DivNode  const& node, int depth) {
    std::cout << "DivNode\n";
    boost::apply_visitor(print_node_visitor{depth+1}, node.node);
}

void print_node(AssignNode const& node, int depth) {
    std::cout << "Assign node\n";

    boost::apply_visitor(print_node_visitor{depth+1}, node.lhs);
    boost::apply_visitor(print_node_visitor{depth+1}, node.rhs);
}

void print_node(LogicAndNode const& node, int depth) {
    std::cout << "Assign node\n";

    boost::apply_visitor(print_node_visitor{depth+1}, node.lhs);
    boost::apply_visitor(print_node_visitor{depth+1}, node.rhs);
}

void print_node(NumberNode const& node, int ) {
    std::cout << "Number node value: " << node.value << "\n";
}

void print_node(StringNode const& node, int ) {
    std::cout << "String node value " << node.value << "\n";
}

void print_node(IdentifierNode const& node, int ) {
    std::cout << "Identifier: " << node.identifier << "\n";
}

void print_node(FnCallNode const& node, int) {
    std::cout << "Function call: " << node.identifier << "\n";
}

void print_node(ParameterNode const& node, int ) {
    std::cout << "Param: ";
    print_node(node.type, 0);
    std::cout << " " << node.name << "\n";
}

void print_ast(AstNode const& node) {
    boost::apply_visitor(print_node_visitor{0}, node);
}


template<typename T> void visit(FileNode const& node, T fn) {
    fn(node);

    for(auto const& m : node.modules)
        visit(m, fn);
}

template<typename T> void visit(ModuleNode const& node, T fn) {
    fn(node);

    for(auto const& f : node.functions)
        visit(f, fn);
}

template<typename T> void visit(FunctionNode const& node, T fn) {
    fn(node);
}


template<typename Tnode, typename Tvisit>
inline typename std::enable_if<!std::is_same<Tnode, AstNode>::value, void>::type
visit(Tnode const& node, Tvisit) {
    std::cout << "shouldnt be here " << typeid(node).name() << "\n";
}

template<typename Tnode, typename Tvisit>
inline typename std::enable_if<std::is_same<Tnode, AstNode>::value, void>::type
visit(Tnode const& node, Tvisit fn) {
    boost::apply_visitor([&](auto const& n) {
        visit(n, fn);
    }, node);
}


#endif //__parser_h__
