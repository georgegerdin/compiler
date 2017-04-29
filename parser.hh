#ifndef __parser_h__
#define __parser_h__

struct FileNode;
struct ModuleNode;
struct FunctionNode;
struct BlockNode;
struct StatementNode;
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

enum TYPES : int {
    TYPE_INT=1,
    TYPE_UINT
};

typedef boost::variant< boost::recursive_wrapper<FileNode>,
                        boost::recursive_wrapper<ModuleNode>,
                        boost::recursive_wrapper<FunctionNode>,
                        boost::recursive_wrapper<BlockNode>,
                        boost::recursive_wrapper<StatementNode>,
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
                        boost::recursive_wrapper<StringNode>
                        > AstNode;

using std::vector;
using std::map;
struct FileNode { map<std::string, AstNode> modules; };
struct ModuleNode { std::string name; vector<AstNode> functions; ModuleNode(std::string iname) : name(iname) {} };
struct TypeNode { int type; };
struct BlockNode { vector<StatementNode> statements; };
struct StatementNode { AstNode expr; };
struct FunctionNode { std::string name; TypeNode return_type; vector<AstNode> parameters; BlockNode func_body;};
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


void tab(int depth) {
    for(int i = 0; i < depth; ++i)
        std::cout << "    ";
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

struct print_node_visitor : public boost::static_visitor<> {
    print_node_visitor(int d) : m_depth(d) {}
    template <typename T>
    void operator()(T node) const { tab(m_depth); print_node(node, m_depth); }
    int m_depth;
};

void print_node(FileNode const& node, int depth) {
    std::cout << "File node\n";

    for(auto& child : node.modules)
        boost::apply_visitor(print_node_visitor{depth+1}, child.second);
}

void print_node(ModuleNode const& node, int depth) {
    std::cout << "Module node: " << node.name << "\n";

    for(auto& children : node.functions)
        boost::apply_visitor(print_node_visitor{depth+1}, children);
}

void print_node(FunctionNode const& node, int depth) {
    std::cout << "Function node: " << node.name << "return type";
    print_node(node.return_type, depth);
    std::cout << "\n";
    print_node(node.func_body, depth+1);
}

void print_node(BlockNode const& node, int depth) {
    std::cout << "Block node: \n";
    for(auto& statement : node.statements) {
        tab(depth); print_node(statement, depth+1);
    }
}

void print_node(StatementNode const& node, int depth) {
    std::cout << "Statement\n";
    boost::apply_visitor(print_node_visitor{depth+1}, node.expr);
}

void print_node(TypeNode const& node, int) {
    switch(node.type) {
        case TYPE_INT:
            std::cout << "int"; break;
        case TYPE_UINT:
            std::cout << "uint"; break;
        default:
            break;
    }
}

void print_node(LetNode const& node, int depth) {
    std::cout << "Let node " << node.var_name << "\n";
    boost::apply_visitor(print_node_visitor{depth+1}, node.rhs);
}

void print_node(ExprNode const& node, int depth) {
    std::cout << "Expression\n";
    for(auto& op : node.operations) {
        tab(depth); boost::apply_visitor(print_node_visitor{depth+1}, op);
    }
}

void print_node(AddNode const& node, int depth) {
    std::cout << "AddNode\n";
    tab(depth); boost::apply_visitor(print_node_visitor{depth+1}, node.node);;
}

void print_node(DecNode  const& node, int depth) {
    std::cout << "DecNode\n";
    tab(depth); boost::apply_visitor(print_node_visitor{depth+1}, node.node);
}

void print_node(MulNode  const& node, int depth) {
    std::cout << "MulNode\n";
    tab(depth); boost::apply_visitor(print_node_visitor{depth+1}, node.node);
}

void print_node(DivNode  const& node, int depth) {
    std::cout << "DivNode\n";
    tab(depth); boost::apply_visitor(print_node_visitor{depth+1}, node.node);
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

void print_ast(AstNode const& node) {
    boost::apply_visitor(print_node_visitor{0}, node);
}

#endif //__parser_h__
