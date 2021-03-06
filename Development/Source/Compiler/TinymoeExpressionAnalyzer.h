#ifndef VCZH_COMPILER_TINYMOEEXPRESSIONANALYZER
#define VCZH_COMPILER_TINYMOEEXPRESSIONANALYZER

#include "TinymoeLexicalAnalyzer.h"

namespace tinymoe
{
	namespace ast
	{
		class AstNode;
		class AstExpression;
		class AstLambdaExpression;
		class AstDeclaration;
	}

	namespace compiler
	{
		class SymbolModule;
		class SymbolAstScope;
		struct SymbolAstContext;
		struct SymbolAstResult;
		class FunctionFragment;

		/*************************************************************
		Symbol
		*************************************************************/

		enum class GrammarFragmentType
		{
			Name,					// for identifier list,				e.g. [repeat with] the current number [from] 1 [to] 100
			Type,					// for type name,					e.g. set names to new [hash set]
			Primitive,				// for primitive expression,		e.g. sum from 1 to [10]
			Expression,				// for all kinds of expressions,	e.g. repeat with the current number from [1] to [100]
			List,					// for tuple (marshalled as array),	e.g. set names to collection of [("a", "b", "c")]
			Assignable,				// for variable, or create a new symbolif the <assignable> does not exist
			//															e.g. [a variable]
			Argument,				// always create a new symbol in the block body
			//															e.g. repeat with [the current number] from 1 to sum from 1 to 10
		};

		class GrammarFragment
		{
		public:
			typedef shared_ptr<GrammarFragment>			Ptr;
			typedef vector<Ptr>							List;

			GrammarFragmentType							type;
			vector<string_t>							identifiers;
			shared_ptr<FunctionFragment>				functionFragment;

			GrammarFragment(GrammarFragmentType _type);

			string_t									GetUniqueIdFragment();
		};

		enum class GrammarSymbolTarget
		{
			Custom,					// user defined symbol

			Object,					// (type)		object
			Array,					// (type)		array
			Symbol,					// (type)		symbol
			Boolean,				// (type)		boolean
			Integer,				// (type)		integer
			Float,					// (type)		float
			String,					// (type)		string_t
			Function,				// (type)		function

			True,					// (primitive)	true
			False,					// (primitive)	false
			Null,					// (primitive)	null
			TheResult,				// (primitive)	the result

			Invoke,					// (primitive)	<primitive> of <list>
			InvokeContinuation,		// (primitive)	continuation <expression> of <list>
			NewTypeOfFields,		// (primitive)	new <type> of <list>
			NewArray,				// (primitive)	new array of <expression> items
			GetArrayItem,			// (primitive)	item <expression> of array <primitive>
			GetArrayLength,			// (primitive)	length of array <primitive>
			IsType,					// (primitive)	<primitive> is <type>
			IsNotType,				// (primitive)	<primitive> is not <type>
			GetField,				// (primitive)	field <argument> of <primitive>

			End,					// (sentence)	end
			Select,					// (block)		select <expression>
			Case,					// (sentence)	case <expression>
			CaseElse,				// (sentence)	case else
			RedirectTo,				// (sentence)	redirect to <expression>
			Assign,					// (sentence)	set <assignable> to <expression>
			SetArrayItem,			// (sentence)	set item <expression> of array <expression> to <expression>
			SetField,				// (sentence)	set field <argument> of <expression> to <expression>
		};

		enum class GrammarSymbolType : int
		{
			Type = 1,				// <type>
			Symbol = 2,				// <primitive>
			Phrase = 4,				// <primitive>
			Sentence = 8,			// <sentence>
			Block = 16,				// <block>
		};

		class GrammarSymbol
		{
		public:
			typedef shared_ptr<GrammarSymbol>			Ptr;
			typedef vector<Ptr>							List;
			typedef multimap<string_t, Ptr>				MultiMap;

			GrammarFragment::List						fragments;		// grammar fragments for this symbol
			// a statement cannot be an expression
			// the top invoke expression's function of a statement should reference to a statement symbol
			string_t									uniqueId;		// a string_t that identifies the grammar structure
			GrammarSymbolTarget							target;
			GrammarSymbolType							type;

			GrammarSymbol(GrammarSymbolType _type, GrammarSymbolTarget _target = GrammarSymbolTarget::Custom);

			void										CalculateUniqueId();
		};

		GrammarSymbol::Ptr								operator+(GrammarSymbol::Ptr symbol, const string_t& name);
		GrammarSymbol::Ptr								operator+(GrammarSymbol::Ptr symbol, GrammarFragmentType type);

		/*************************************************************
		Expression
		*************************************************************/

		class Expression : public CodeFragment
		{
		public:
			typedef shared_ptr<Expression>				Ptr;
			typedef vector<Ptr>							List;

			virtual string_t							ToLog() = 0;
			virtual string_t							ToCode() = 0;
			virtual void								CollectNewAssignable(Expression::List& newAssignables, Expression::List& newArguments, Expression::List& modifiedAssignables) = 0;
			virtual SymbolAstResult						GenerateAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state) = 0;
			static shared_ptr<ast::AstLambdaExpression>	GenerateContinuationLambdaAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state);
		};

		// for numbers and strings
		class LiteralExpression : public Expression
		{
		public:
			CodeToken									token;

			string_t									ToLog()override;
			string_t									ToCode()override;
			void										CollectNewAssignable(Expression::List& newAssignables, Expression::List& newArguments, Expression::List& modifiedAssignables)override;
			SymbolAstResult								GenerateAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state)override;
		};

		// for new created symbols in <assignable> and <argument>
		class ArgumentExpression : public Expression
		{
		public:
			SymbolName::Ptr								name;

			string_t									ToLog()override;
			string_t									ToCode()override;
			void										CollectNewAssignable(Expression::List& newAssignables, Expression::List& newArguments, Expression::List& modifiedAssignables)override;
			SymbolAstResult								GenerateAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state)override;
		};

		// for symbol referencing
		class ReferenceExpression : public Expression
		{
		public:
			GrammarSymbol::Ptr							symbol;

			string_t									ToLog()override;
			string_t									ToCode()override;
			void										CollectNewAssignable(Expression::List& newAssignables, Expression::List& newArguments, Expression::List& modifiedAssignables)override;
			SymbolAstResult								GenerateAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state)override;
		};

		// for function invoking
		class InvokeExpression : public Expression
		{
		public:
			typedef shared_ptr<InvokeExpression>		Ptr;

			Expression::Ptr								function;
			Expression::List							arguments;

			string_t									ToLog()override;
			string_t									ToCode()override;
			void										CollectNewAssignable(Expression::List& newAssignables, Expression::List& newArguments, Expression::List& modifiedAssignables)override;
			SymbolAstResult								GenerateAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state)override;
		};

		// for <list>
		class ListExpression : public Expression
		{
		public:
			Expression::List							elements;

			string_t									ToLog()override;
			string_t									ToCode()override;
			void										CollectNewAssignable(Expression::List& newAssignables, Expression::List& newArguments, Expression::List& modifiedAssignables)override;
			SymbolAstResult								GenerateAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state)override;
		};

		enum class UnaryOperator
		{
			Positive,
			Negative,
			Not,
		};

		// for unary operator invoking
		class UnaryExpression : public Expression
		{
		public:
			Expression::Ptr								operand;
			UnaryOperator								op;

			string_t									ToLog()override;
			string_t									ToCode()override;
			void										CollectNewAssignable(Expression::List& newAssignables, Expression::List& newArguments, Expression::List& modifiedAssignables)override;
			SymbolAstResult								GenerateAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state)override;
		};

		enum class BinaryOperator
		{
			Concat,
			Add,
			Sub,
			Mul,
			Div,
			IntDiv,
			Mod,
			LT,
			GT,
			LE,
			GE,
			EQ,
			NE,
			And,
			Or,
		};

		// for binary operator invoking
		class BinaryExpression : public Expression
		{
		public:
			Expression::Ptr								first;
			Expression::Ptr								second;
			BinaryOperator								op;

			string_t									ToLog()override;
			string_t									ToCode()override;
			void										CollectNewAssignable(Expression::List& assignables, Expression::List& arguments, Expression::List& modifiedAssignables)override;
			SymbolAstResult								GenerateAst(shared_ptr<SymbolAstScope> scope, SymbolAstContext& context, shared_ptr<ast::AstDeclaration> state)override;
		};

		/*************************************************************
		Symbol Stack
		*************************************************************/

		class GrammarStackItem
		{
		public:
			typedef shared_ptr<GrammarStackItem>		Ptr;
			typedef vector<Ptr>							List;

			GrammarSymbol::List							symbols;

			void										FillPredefinedSymbols();
		};

		class GrammarStack
		{
		public:
			typedef shared_ptr<GrammarStack>			Ptr;
			typedef CodeToken::List::iterator			Iterator;
			typedef pair<Iterator, Expression::Ptr>		ResultItem;
			typedef vector<ResultItem>					ResultList;
			typedef CodeError(GrammarStack::* ParseFunctionType)(Iterator, Iterator, ResultList&);

			GrammarStackItem::List						stackItems;				// available symbols organized in a scope based structure
			GrammarSymbol::MultiMap						availableSymbols;		// available symbols grouped by the unique identifier
			GrammarSymbol::Ptr							resultSymbol;
			// the last symbol overrides all other symbols in the same group

			struct ExpressionLink
			{
				typedef shared_ptr<ExpressionLink>		Ptr;

				Expression::Ptr							expression;
				Ptr										previous;
			};

			void										Push(GrammarStackItem::Ptr stackItem);
			GrammarStackItem::Ptr						Pop();

			CodeError									SuccessError();
			CodeError									ParseToken(const string_t& token, Iterator input, Iterator end, vector<Iterator>& result);
			CodeError									FoldError(CodeError error1, CodeError error2);

			CodeError									ParseGrammarFragment(GrammarFragment::Ptr fragment, Iterator input, Iterator end, ResultList& result);
			CodeError									ParseGrammarSymbolStep(GrammarSymbol::Ptr symbol, int fragmentIndex, ExpressionLink::Ptr previousExpression, Iterator input, Iterator end, vector<pair<Iterator, ExpressionLink::Ptr>>& result);
			CodeError									ParseGrammarSymbol(GrammarSymbol::Ptr symbol, int beginFragment, ExpressionLink::Ptr previousExpression, Iterator input, Iterator end, ResultList& result);
			CodeError									ParseGrammarSymbol(GrammarSymbol::Ptr symbol, Iterator input, Iterator end, ResultList& result);

			CodeError									ParseType(Iterator input, Iterator end, ResultList& result);			// <type>
			CodeError									ParseShortPrimitive(Iterator input, Iterator end, ResultList& result);	// <literal>, op <primitive>, (<expression>), <phrase>
			CodeError									ParsePrimitive(Iterator input, Iterator end, ResultList& result);		// left recursive <phrase>
			CodeError									ParseList(Iterator input, Iterator end, ResultList& result);			// (<expression>, ...)
			CodeError									ParseAssignable(Iterator input, Iterator end, ResultList& result);		// <symbol> or <argument>
			CodeError									ParseArgument(Iterator input, Iterator end, ResultList& result);		// <argument>

			CodeError									ParseBinary(Iterator input, Iterator end, ParseFunctionType parser, CodeTokenType* tokenTypes, BinaryOperator* binaryOperators, int count, ResultList& result);
			CodeError									ParseExp1(Iterator input, Iterator end, ResultList& result);			// * /
			CodeError									ParseExp2(Iterator input, Iterator end, ResultList& result);			// + -
			CodeError									ParseExp3(Iterator input, Iterator end, ResultList& result);			// &
			CodeError									ParseExp4(Iterator input, Iterator end, ResultList& result);			// < > <= >= = <>
			CodeError									ParseExp5(Iterator input, Iterator end, ResultList& result);			// and
			CodeError									ParseExpression(Iterator input, Iterator end, ResultList& result);		// or, aka. <expression>

			CodeError									ParseStatement(Iterator input, Iterator end, ResultList& result);
			int											CountStatementAssignables(Expression::List& assignables);				// -1: illegal assignable (e.g. the assignable is a legal expression)
			int											CountStatementAssignables(Expression::List& assignables, Expression::Ptr& illegalConvertedAssignable);
		};
	}
}

#endif