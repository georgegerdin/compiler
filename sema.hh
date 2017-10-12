#ifndef semanticanalysis_h__
#define semanticanalysis_h__

const char* reserved[] = {
    "fn",
    "module",
    "if",
    "let",
    "return",
    "void",
    "int"
};

class Sema {
public:
    Sema(AstNode& node, SymbolTable& sym) : m_ast(node), m_sym(sym) {}

    bool Analyse();

protected:
    bool Analysis(AstNode const& node, SymbolPath path);
    bool Analysis(FileNode const& node, SymbolPath path);
    bool Analysis(ModuleNode const& node, SymbolPath path);
    bool Analysis(FunctionNode const& node, SymbolPath path);
    bool Analysis(BlockNode const& node, SymbolPath path);
    bool Analysis(StatementNode const&, SymbolPath path);
    bool Analysis(LetNode const&, SymbolPath path);

    bool LegalSymbolName(const std::string& name);

    void Error(std::string error_str) { std::cout << error_str << std::endl; };

    template<typename T>
    bool Analysis(T const&, SymbolPath) {
        return true;
    }

    AstNode& m_ast;
    SymbolTable& m_sym;
};

bool Sema::Analyse()
{
    return Analysis(m_ast, SymbolPath{});
}

bool Sema::Analysis(AstNode const& node, SymbolPath path)
{
    auto visitor = [&](auto const& n) {return Analysis(n, path);};
    return boost::apply_visitor(visitor, node);
}

bool Sema::Analysis(FileNode const& node, SymbolPath path)
{
    bool result = true;
    for(const ModuleNode& m : node.modules) {
        if(!Analysis(m, path))
            result = false;
    }

    return result;
}

bool Sema::Analysis(ModuleNode const& node, SymbolPath path)
{
    bool result = true;

    if(node.name) {
        path.emplace_back(node.name.get());
    }

    for(const AstNode& fn : node.functions) {
        if(!Analysis(fn, path))
            result = false;
    }
    return result;
}

bool Sema::Analysis(FunctionNode const& node, SymbolPath path)
{
    path.emplace_back(node.name);

    for(auto const& param : node.parameters) {
        Analysis(param.type, path);
        if(!LegalSymbolName(param.name)) {
            Error("Semantic error, reserved keyword");
            return false;
        }
    }

    if(!Analysis(node.return_type, path))
        return false;
    return Analysis(node.func_body, path);
}

bool Sema::Analysis(BlockNode const& node, SymbolPath path) {
    for(auto const& s : node.statements) {
        if(!Analysis(s, path))
            return false;
    }
    return true;
}

bool Sema::Analysis(StatementNode const& node, SymbolPath path)
{
    return Analysis(node.expr, path);
}

bool Sema::Analysis(LetNode const& node, SymbolPath path)
{
    if(!LegalSymbolName(node.var_name)) {
        Error("Semantic error, reserved keyword not allowed as variable name");
        return false;
    }

    return Analysis(node.rhs, path);
}

bool Sema::LegalSymbolName(std::string const& name)
{
    for(auto const& r : reserved) {
        if(name == r)
            return false;
    }
    return true;
}



#endif//semanticanalysis_h__
