#ifndef semanticanalysis_h__
#define semanticanalysis_h__

#include "expected.hh"

const char* reserved[] = {
    "fn",
    "module",
    "if",
    "let",
    "return",
    "void",
    "int"
};

struct ReservedKeyword { std::string keyword; };
struct InvalidFunctionReturnType { std::string invalid_type; };
struct UndefinedSymbol { std::string symbol; };
struct InvalidSymbol { std::string symbol; };
struct InvalidBlock {};
struct SymbolAlreadyDefined{std::string symbol;};
using SemaError = boost::variant<ReservedKeyword, InvalidFunctionReturnType, UndefinedSymbol, InvalidSymbol, InvalidBlock, SymbolAlreadyDefined>;

using Result = nonstd::expected<SymbolTable::Category, SemaError>;

class Sema {
public:
    Sema(AstNode& node, SymbolTable& sym) : m_ast(node), m_sym(sym) {}

    Result Analyse();

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
	Result Analysis(ExprNode const&, SymbolPath path);
	Result Analysis(IdentifierNode const&, SymbolPath path);

    bool LegalSymbolName(const std::string& name);

    template<typename T>
    Result Analysis(T const&, SymbolPath) {
        return SymbolTable::Category{SymbolTable::Variable{}};
    }

    AstNode& m_ast;
    SymbolTable& m_sym;
};

Result Sema::Analyse()
{
	auto ai = Analysis(m_ast, SymbolPath{});
	return ai;
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
		auto ai = Analysis(m, path);
        if(!ai)
            result = ai;
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
		auto ai = Analysis(fn, path);
        if(!ai)
            result = ai;
    }
    return result;
}

Result Sema::Analysis(FunctionNode const& node, SymbolPath path)
{
    path.emplace_back(node.name);

    for(auto const& param : node.parameters) {
        Analysis(param.type, path);
        if(!LegalSymbolName(param.name)) {
			//Parameters are not allowed to be named reserved keywords
			return nonstd::make_unexpected(ReservedKeyword{ param.name } );
        }
    }

    if(!Analysis(node.return_type, path)) {
		//Invalid function return type
		return nonstd::make_unexpected(InvalidFunctionReturnType{ "test" });
    }
    return Analysis(node.func_body, path);
}

Result Sema::Analysis(BlockNode const& node, SymbolPath path) {
	Result last_result = nonstd::make_unexpected(InvalidBlock{});
    for(auto const& s : node.statements) {
        last_result = Analysis(s, path);
        if(!last_result)
            return last_result;
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
		return nonstd::make_unexpected(ReservedKeyword{ node.var_name });
    }

    auto rhs_result = Analysis(node.rhs, path);
	if (!rhs_result) {
		return rhs_result;
	}

    auto symbols = m_sym.Lookup(node.var_name);
    if(!symbols) {
		return nonstd::make_unexpected(UndefinedSymbol{ node.var_name });
    }

    auto sym_it = symbols->begin();
    SymbolTable::Entry* sym_ptr = *sym_it;
    SymbolTable::Variable* var;
    if(!(var = boost::get<SymbolTable::Variable>(&sym_ptr->category))) {
		return nonstd::make_unexpected(InvalidSymbol{ node.var_name });
    }

    return boost::apply_visitor(boost::hana::overload(
        [&](SymbolTable::Auto const& v) -> Result {
            sym_ptr->category = rhs_result.value();
            return rhs_result;
        },
        [&](auto const&) -> Result {
			return nonstd::make_unexpected(SymbolAlreadyDefined{ node.var_name });
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
				return nonstd::make_unexpected(InvalidSymbol{ nt.name });
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


Result Sema::Analysis(ExprNode const& en, SymbolPath path)
{
	Result result;
	boost::optional<SymbolTable::Category> category;

	for (const auto& op : en.operations) {
		result = Analysis(op, path);
		if (!result)
			return result;
		if (!category)
			category = *result;
		else if()
	}
	return result;
}

Result Sema::Analysis(AddNode const& an, SymbolPath path) {
	return Analysis(an, path);
}

Result Sema::Analysis(IdentifierNode const& in, SymbolPath path)
{
	auto lookup = m_sym.Lookup(in.identifier);
	if (!lookup)
		return nonstd::make_unexpected(UndefinedSymbol{ in.identifier });

	auto& result = *lookup;
	return result[0]->category;
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
