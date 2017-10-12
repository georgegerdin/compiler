#ifndef __SYMBOLTABLE_HH__
#define __SYMBOLTABLE_HH__

using SymbolPath = std::vector<std::string>;

class SymbolTable {
public:
    SymbolTable(AstNode& root) : m_root_ast_node(root) {}

    void Generate();
    void Generate(AstNode const& node, SymbolPath = SymbolPath {} );
    void Generate(FileNode const& fn, SymbolPath path);
    void Generate(ModuleNode const& mn, SymbolPath path);
    void Generate(FunctionNode const& fn, SymbolPath path);
    void Generate(ParameterNode const& pn, SymbolPath path);
    void Generate(BlockNode const& bn, SymbolPath path);
    void Generate(StatementNode const& sn, SymbolPath path);
    void Generate(LetNode const& ln, SymbolPath path);
    
    template <typename T>
    void Generate(T const& tn) {
    }
    
    enum Category {
        Namespace,
        Module,
        Function,
        Variable
    };

    enum BuiltinType {
        Int,
        Uint,
        Void
    };

    struct UserDefinedType {
        std::string name;
    };

    using Type = boost::variant<BuiltinType, UserDefinedType>;
    
    struct Entry {
        int line;
        int column;
        Category category;
        Type type;
        SymbolPath path;
    };

    boost::optional<std::vector<Entry*>> Lookup(const char* symbol);

    using SymbolMap = std::multimap<std::string, Entry>;
    SymbolMap m_symbols;
    AstNode& m_root_ast_node;
};

boost::optional<std::vector<SymbolTable::Entry*>> SymbolTable::Lookup(const char *symbol)
{
    auto symbols = m_symbols.find(symbol);
    if(symbols == m_symbols.end())
        return boost::none;

    std::vector<Entry*> result;
    while(symbols != m_symbols.end()) {
        result.emplace_back(&symbols->second);
        ++symbols;
    }
    return result;
}

void SymbolTable::Generate() {
    Generate(m_root_ast_node);
}

SymbolTable::Entry make_entry(SymbolTable::Category symbol_type, SymbolPath symbol_path) {
    SymbolTable::Entry entry;
    entry.category = symbol_type;
    entry.path = symbol_path;
    return entry;
}

void SymbolTable::Generate(FileNode const& fn, SymbolPath path) {
    for(auto const& module : fn.modules) {
        Generate(module, path);
    }
}

void SymbolTable::Generate(ModuleNode const& mn, SymbolPath path) {
    if(mn.name) {
        m_symbols.emplace(mn.name.get(), make_entry(Category::Module, path));
        path.push_back(mn.name.get());
    }
    for(auto const& func : mn.functions) {
        Generate(func, path);
    }
}

void SymbolTable::Generate(FunctionNode const& fn, SymbolPath path) {
     m_symbols.emplace(fn.name, make_entry(Category::Function, path));
     
    path.push_back(fn.name);

    for(ParameterNode const& param : fn.parameters) {
        Generate(param, path);
    }
     
    Generate(fn.func_body, path);
}

void SymbolTable::Generate(ParameterNode const& pn, SymbolPath path) {
    m_symbols.emplace(pn.name, make_entry(Category::Variable, path));
}

void SymbolTable::Generate(BlockNode const& bn, SymbolPath path) {
    for(auto const& statement : bn.statements) {
        Generate(statement, path);
    }
}

void SymbolTable::Generate(StatementNode const& sn, SymbolPath path) {
    Generate(sn.expr, path);
}
    
void SymbolTable::Generate(LetNode const& ln, SymbolPath path) {
    m_symbols.emplace(ln.var_name, make_entry(Category::Variable, path));
}

void SymbolTable::Generate(AstNode const& node, SymbolPath path) {
    boost::apply_visitor(
        [this, &path](auto const& n) {
            Generate(n, path);
        },
        node);
}

void print_symbol_table(SymbolTable const& table) {
    std::cout << "SYMBOLS:\n";
    for(auto const& symbol : table.m_symbols) {
        std::cout << "\t" << symbol.first << "\t";
        switch(symbol.second.category) {
            case SymbolTable::Category::Namespace:
                std::cout << "namespace";break;
            case SymbolTable::Category::Module:
                std::cout << "module";break;
            case SymbolTable::Category::Function:
                std::cout << "function";break;
            case SymbolTable::Category::Variable:
                std::cout << "variable";break;
        }
        std::cout << "\t"; 
        for(auto const& a : symbol.second.path) {
            std::cout << "::" << a;
        }
        std::cout << "\n";
    }
}

#endif
