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

using Result = boost::optional<SymbolTable::Category>;

class Sema {
public:
    Sema(AstNode& node, SymbolTable& sym) : m_ast(node), m_sym(sym) {}

    bool Analyse();

protected:
    Result Analysis(AstNode const& node, SymbolPath path);
    Result Analysis(FileNode const& node, SymbolPath path);
    Result Analysis(ModuleNode const& node, SymbolPath path);
    Result Analysis(FunctionNode const& node, SymbolPath path);
    Result Analysis(BlockNode const& node, SymbolPath path);
    Result Analysis(StatementNode const&, SymbolPath path);
    Result Analysis(LetNode const&, SymbolPath path);
    Result Analysis(TypeNode const&, SymbolPath path);
    Result Analysis(NumberNode const& nn, SymbolPath);
    bool LegalSymbolName(const std::string& name);

    void Error(std::string error_str) { std::cout << error_str << std::endl; };

    template<typename T>
    Result Analysis(T const&, SymbolPath) {
        return SymbolTable::Category{SymbolTable::Variable{}};
    }

    AstNode& m_ast;
    SymbolTable& m_sym;
};

bool Sema::Analyse()
{
    return Analysis(m_ast, SymbolPath{}) != boost::none;
}

Result Sema::Analysis(AstNode const& node, SymbolPath path)
{
    auto visitor = [&](auto const& n) {return Analysis(n, path);};
    return boost::apply_visitor(visitor, node);
}

Result Sema::Analysis(FileNode const& node, SymbolPath path)
{
    Result result = SymbolTable::Category{SymbolTable::File{}};
    for(const ModuleNode& m : node.modules) {
        if(!Analysis(m, path))
            result = boost::none;
    }

    return result;
}

Result Sema::Analysis(ModuleNode const& node, SymbolPath path)
{
    Result result = SymbolTable::Category{SymbolTable::Module{}};

    if(node.name) {
        path.emplace_back(node.name.get());
    }

    for(const AstNode& fn : node.functions) {
        if(!Analysis(fn, path))
            result = boost::none;
    }
    return result;
}

Result Sema::Analysis(FunctionNode const& node, SymbolPath path)
{
    path.emplace_back(node.name);

    for(auto const& param : node.parameters) {
        Analysis(param.type, path);
        if(!LegalSymbolName(param.name)) {
            Error("Semantic error, reserved keyword");
            return boost::none;
        }
    }

    if(!Analysis(node.return_type, path)) {
        Error("Semantic error, invalid function return type");
        return boost::none;
    }
    return Analysis(node.func_body, path);
}

Result Sema::Analysis(BlockNode const& node, SymbolPath path) {
    Result last_result = boost::none;
    for(auto const& s : node.statements) {
        last_result = Analysis(s, path);
        if(!last_result)
            return boost::none;
    }
    return last_result;
}

Result Sema::Analysis(StatementNode const& node, SymbolPath path)
{
    return Analysis(node.expr, path);
}

Result Sema::Analysis(LetNode const& node, SymbolPath path)
{
    if(!LegalSymbolName(node.var_name)) {
        Error("Semantic error, reserved keyword not allowed as variable name");
        return boost::none;
    }

    auto rhs_result = Analysis(node.rhs, path);

    auto symbols = m_sym.Lookup(node.var_name);
    if(!symbols) {
        Error("Semantic error, variable not in the symbols table");
        return boost::none;
    }

    auto sym_it = symbols->begin();
    SymbolTable::Entry* sym_ptr = *sym_it;
    SymbolTable::Variable* var;
    if(!(var = boost::get<SymbolTable::Variable>(&sym_ptr->category))) {
        Error("Semantic error, symbol not registered as variable");
        return boost::none;
    }

    return boost::apply_visitor(boost::hana::overload(
        [&](SymbolTable::Auto const& v) -> Result {
            sym_ptr->category = rhs_result.get();
            return rhs_result;
        },
        [](auto const&) -> Result {
            Error("Semantic error, variable already has a type");
            return boost::none;
        }), var->type);
}

Result Sema::Analysis(TypeNode const& tn, SymbolPath path) {
    return boost::apply_visitor(boost::hana::overload(
        [](SimpleType const& st) -> Result {
            return SymbolTable::Category{
                    SymbolTable::Variable{SymbolTable::BuiltinType::Int}};
        },
        [this](NamedType const& nt) -> Result{
            if(!m_sym.Lookup(nt.name)) {
                Error("Semantic error, type not defined");
                return boost::none;
            }
            return SymbolTable::Category{
                SymbolTable::Variable{
                    SymbolTable::UserDefinedType{nt.name}}};
        }),
        tn.type);
}

Result Sema::Analysis(NumberNode const& nn, SymbolPath) {
    std::cout << "Number " << nn.value << "\n";
    return SymbolTable::Category{
        SymbolTable::Variable{SymbolTable::BuiltinType{
                SymbolTable::BuiltinType::Int}}};
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
