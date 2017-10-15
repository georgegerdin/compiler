#ifndef __SYMBOLTABLE_HH__
#define __SYMBOLTABLE_HH__

using SymbolPath = std::vector<std::string>;

class SymbolTable {
public:
    SymbolTable(AstNode& root) : m_root_ast_node(root) {}

    void Generate();
    void Generate(AstNode const& node, SymbolPath = SymbolPath {} );

    struct Auto {};

    enum BuiltinType {
        Int,
        Uint,
        Void
    };

    struct UserDefinedType {
        std::string name;
    };

    using Type = boost::variant<
        Auto,
        BuiltinType,
        UserDefinedType>;

    struct File {};
    struct Namespace {};
    struct Module {};
    struct Function {Type type = Auto{};};
    struct Variable {Type type = Auto{};};

    using Category = boost::variant<
        File,
        Namespace,
        Module,
        Function,
        Variable
    >;


    struct Entry {
        int line;
        int column;
        Category category;
        SymbolPath path;
        AstNode const* node_ptr = nullptr;
    };

    boost::optional<std::vector<Entry*>> Lookup(const char* symbol);
    boost::optional<std::vector<Entry*>> Lookup(const std::string& symbol);

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

boost::optional<std::vector<SymbolTable::Entry*>> SymbolTable::Lookup(const std::string& symbol) {
    return Lookup(symbol.c_str());
}

void SymbolTable::Generate() {
    Generate(m_root_ast_node);
}

SymbolTable::Entry make_entry(SymbolTable::Category symbol_type, SymbolPath symbol_path, AstNode const* ptr = nullptr) {
    SymbolTable::Entry entry;
    entry.category = symbol_type;
    entry.path = symbol_path;
    entry.node_ptr = ptr;
    return entry;
}

void SymbolTable::Generate(AstNode const& node, SymbolPath path) {
    auto gen_visitor = boost::hana::overload(
        [&](LetNode const& ln) {
            m_symbols.emplace(ln.var_name, make_entry(Variable{}, path, &node));
        },
        [&](StatementNode const& sn) {
            Generate(sn.expr, path);
        },
        [&](BlockNode const& bn) {
            for(auto const& statement : bn.statements) {
                Generate(statement, path);
            }
        },
        [&](ParameterNode const& pn) {
            m_symbols.emplace(pn.name, make_entry(Variable{}, path, &node));
        },
        [&](FunctionNode const& fn) {
            m_symbols.emplace(fn.name, make_entry(Function{}, path, &node));

           path.push_back(fn.name);

           for(ParameterNode const& param : fn.parameters) {
               Generate(param, path);
           }

           Generate(fn.func_body, path);
        },
        [&](ModuleNode const & mn) {
            if(mn.name) {
                m_symbols.emplace(mn.name.get(), make_entry(Module{}, path, &node));
                path.push_back(mn.name.get());
            }
            for(auto const& func : mn.functions) {
                Generate(func, path);
            }
        },
        [&](FileNode const& fn) {
            for(auto const& module : fn.modules) {
                Generate(module, path);
            }
        },
        [&](auto const&) {}
    );

    boost::apply_visitor(gen_visitor, node);
}

void print_symbol_table(SymbolTable const& table) {
    std::cout << "SYMBOLS:\n";
    for(auto const& symbol : table.m_symbols) {
        std::cout << "\t" << symbol.first << "\t";
        boost::apply_visitor(boost::hana::overload(
            [](SymbolTable::File const&) {
                std::cout << "file";
            },
            [](SymbolTable::Namespace const&) {
                std::cout << "namespace";
            },
            [](SymbolTable::Module const&) {
                std::cout << "module";
            },
            [](SymbolTable::Function const&) {
                std::cout << "function";
            },
            [](SymbolTable::Variable const&) {
                std::cout << "variable";
            }), symbol.second.category);
        std::cout << "\t";
        for(auto const& a : symbol.second.path) {
            std::cout << "::" << a;
        }
        std::cout << "\n";
    }
}

#endif
