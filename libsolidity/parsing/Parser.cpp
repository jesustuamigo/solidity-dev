/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Solidity parser.
 */

#include <vector>
#include <libdevcore/Log.h>
#include <libevmasm/SourceLocation.h>
#include <libsolidity/parsing/Parser.h>
#include <libsolidity/parsing/Scanner.h>
#include <libsolidity/interface/Exceptions.h>
#include <libsolidity/interface/InterfaceHandler.h>

using namespace std;

namespace dev
{
namespace solidity
{

/// AST node factory that also tracks the begin and end position of an AST node
/// while it is being parsed
class Parser::ASTNodeFactory
{
public:
	ASTNodeFactory(Parser const& _parser):
		m_parser(_parser), m_location(_parser.position(), -1, _parser.sourceName()) {}
	ASTNodeFactory(Parser const& _parser, ASTPointer<ASTNode> const& _childNode):
		m_parser(_parser), m_location(_childNode->location()) {}

	void markEndPosition() { m_location.end = m_parser.endPosition(); }
	void setLocation(SourceLocation const& _location) { m_location = _location; }
	void setLocationEmpty() { m_location.end = m_location.start; }
	/// Set the end position to the one of the given node.
	void setEndPositionFromNode(ASTPointer<ASTNode> const& _node) { m_location.end = _node->location().end; }

	template <class NodeType, typename... Args>
	ASTPointer<NodeType> createNode(Args&& ... _args)
	{
		if (m_location.end < 0)
			markEndPosition();
		return make_shared<NodeType>(m_location, forward<Args>(_args)...);
	}

private:
	Parser const& m_parser;
	SourceLocation m_location;
};

ASTPointer<SourceUnit> Parser::parse(shared_ptr<Scanner> const& _scanner)
{
	try
	{
		m_scanner = _scanner;
		ASTNodeFactory nodeFactory(*this);
		vector<ASTPointer<ASTNode>> nodes;
		while (m_scanner->currentToken() != Token::EOS)
		{
			switch (auto token = m_scanner->currentToken())
			{
			case Token::Import:
				nodes.push_back(parseImportDirective());
				break;
			case Token::Contract:
			case Token::Library:
				nodes.push_back(parseContractDefinition(token == Token::Library));
				break;
			default:
				fatalParserError(std::string("Expected import directive or contract definition."));
			}
		}
		return nodeFactory.createNode<SourceUnit>(nodes);
	}
	catch (FatalError const&)
	{
		if (m_errors.empty())
			throw; // Something is weird here, rather throw again.
		return nullptr;
	}
}

std::shared_ptr<const string> const& Parser::sourceName() const
{
	return m_scanner->sourceName();
}

int Parser::position() const
{
	return m_scanner->currentLocation().start;
}

int Parser::endPosition() const
{
	return m_scanner->currentLocation().end;
}

ASTPointer<ImportDirective> Parser::parseImportDirective()
{
	ASTNodeFactory nodeFactory(*this);
	expectToken(Token::Import);
	if (m_scanner->currentToken() != Token::StringLiteral)
		fatalParserError(std::string("Expected string literal (URL)."));
	ASTPointer<ASTString> url = getLiteralAndAdvance();
	nodeFactory.markEndPosition();
	expectToken(Token::Semicolon);
	return nodeFactory.createNode<ImportDirective>(url);
}

ASTPointer<ContractDefinition> Parser::parseContractDefinition(bool _isLibrary)
{
	ASTNodeFactory nodeFactory(*this);
	ASTPointer<ASTString> docString;
	if (m_scanner->currentCommentLiteral() != "")
		docString = make_shared<ASTString>(m_scanner->currentCommentLiteral());
	expectToken(_isLibrary ? Token::Library : Token::Contract);
	ASTPointer<ASTString> name = expectIdentifierToken();
	vector<ASTPointer<InheritanceSpecifier>> baseContracts;
	if (m_scanner->currentToken() == Token::Is)
		do
		{
			m_scanner->next();
			baseContracts.push_back(parseInheritanceSpecifier());
		}
		while (m_scanner->currentToken() == Token::Comma);
	vector<ASTPointer<ASTNode>> subNodes;
	expectToken(Token::LBrace);
	while (true)
	{
		Token::Value currentTokenValue= m_scanner->currentToken();
		if (currentTokenValue == Token::RBrace)
			break;
		else if (currentTokenValue == Token::Function)
			subNodes.push_back(parseFunctionDefinition(name.get()));
		else if (currentTokenValue == Token::Struct)
			subNodes.push_back(parseStructDefinition());
		else if (currentTokenValue == Token::Enum)
			subNodes.push_back(parseEnumDefinition());
		else if (
			currentTokenValue == Token::Identifier ||
			currentTokenValue == Token::Mapping ||
			Token::isElementaryTypeName(currentTokenValue)
		)
		{
			VarDeclParserOptions options;
			options.isStateVariable = true;
			options.allowInitialValue = true;
			subNodes.push_back(parseVariableDeclaration(options));
			expectToken(Token::Semicolon);
		}
		else if (currentTokenValue == Token::Modifier)
			subNodes.push_back(parseModifierDefinition());
		else if (currentTokenValue == Token::Event)
			subNodes.push_back(parseEventDefinition());
		else if (currentTokenValue == Token::Using)
			subNodes.push_back(parseUsingDirective());
		else
			fatalParserError(std::string("Function, variable, struct or modifier declaration expected."));
	}
	nodeFactory.markEndPosition();
	expectToken(Token::RBrace);
	return nodeFactory.createNode<ContractDefinition>(
		name,
		docString,
		baseContracts,
		subNodes,
		_isLibrary
	);
}

ASTPointer<InheritanceSpecifier> Parser::parseInheritanceSpecifier()
{
	ASTNodeFactory nodeFactory(*this);
	ASTPointer<Identifier> name(parseIdentifier());
	vector<ASTPointer<Expression>> arguments;
	if (m_scanner->currentToken() == Token::LParen)
	{
		m_scanner->next();
		arguments = parseFunctionCallListArguments();
		nodeFactory.markEndPosition();
		expectToken(Token::RParen);
	}
	else
		nodeFactory.setEndPositionFromNode(name);
	return nodeFactory.createNode<InheritanceSpecifier>(name, arguments);
}

Declaration::Visibility Parser::parseVisibilitySpecifier(Token::Value _token)
{
	Declaration::Visibility visibility(Declaration::Visibility::Default);
	if (_token == Token::Public)
		visibility = Declaration::Visibility::Public;
	else if (_token == Token::Internal)
		visibility = Declaration::Visibility::Internal;
	else if (_token == Token::Private)
		visibility = Declaration::Visibility::Private;
	else if (_token == Token::External)
		visibility = Declaration::Visibility::External;
	else
		solAssert(false, "Invalid visibility specifier.");
	m_scanner->next();
	return visibility;
}

ASTPointer<FunctionDefinition> Parser::parseFunctionDefinition(ASTString const* _contractName)
{
	ASTNodeFactory nodeFactory(*this);
	ASTPointer<ASTString> docstring;
	if (m_scanner->currentCommentLiteral() != "")
		docstring = make_shared<ASTString>(m_scanner->currentCommentLiteral());

	expectToken(Token::Function);
	ASTPointer<ASTString> name;
	if (m_scanner->currentToken() == Token::LParen)
		name = make_shared<ASTString>(); // anonymous function
	else
		name = expectIdentifierToken();
	VarDeclParserOptions options;
	options.allowLocationSpecifier = true;
	ASTPointer<ParameterList> parameters(parseParameterList(options));
	bool isDeclaredConst = false;
	Declaration::Visibility visibility(Declaration::Visibility::Default);
	vector<ASTPointer<ModifierInvocation>> modifiers;
	while (true)
	{
		Token::Value token = m_scanner->currentToken();
		if (token == Token::Const)
		{
			isDeclaredConst = true;
			m_scanner->next();
		}
		else if (token == Token::Identifier)
			modifiers.push_back(parseModifierInvocation());
		else if (Token::isVisibilitySpecifier(token))
		{
			if (visibility != Declaration::Visibility::Default)
				fatalParserError(std::string("Multiple visibility specifiers."));
			visibility = parseVisibilitySpecifier(token);
		}
		else
			break;
	}
	ASTPointer<ParameterList> returnParameters;
	if (m_scanner->currentToken() == Token::Returns)
	{
		bool const permitEmptyParameterList = false;
		m_scanner->next();
		returnParameters = parseParameterList(options, permitEmptyParameterList);
	}
	else
		returnParameters = createEmptyParameterList();
	ASTPointer<Block> block = ASTPointer<Block>();
	nodeFactory.markEndPosition();
	if (m_scanner->currentToken() != Token::Semicolon)
	{
		block = parseBlock();
		nodeFactory.setEndPositionFromNode(block);
	}
	else
		m_scanner->next(); // just consume the ';'
	bool const c_isConstructor = (_contractName && *name == *_contractName);
	return nodeFactory.createNode<FunctionDefinition>(
		name,
		visibility,
		c_isConstructor,
		docstring,
		parameters,
		isDeclaredConst,
		modifiers,
		returnParameters,
		block
	);
}

ASTPointer<StructDefinition> Parser::parseStructDefinition()
{
	ASTNodeFactory nodeFactory(*this);
	expectToken(Token::Struct);
	ASTPointer<ASTString> name = expectIdentifierToken();
	vector<ASTPointer<VariableDeclaration>> members;
	expectToken(Token::LBrace);
	while (m_scanner->currentToken() != Token::RBrace)
	{
		members.push_back(parseVariableDeclaration());
		expectToken(Token::Semicolon);
	}
	nodeFactory.markEndPosition();
	expectToken(Token::RBrace);
	return nodeFactory.createNode<StructDefinition>(name, members);
}

ASTPointer<EnumValue> Parser::parseEnumValue()
{
	ASTNodeFactory nodeFactory(*this);
	nodeFactory.markEndPosition();
	return nodeFactory.createNode<EnumValue>(expectIdentifierToken());
}

ASTPointer<EnumDefinition> Parser::parseEnumDefinition()
{
	ASTNodeFactory nodeFactory(*this);
	expectToken(Token::Enum);
	ASTPointer<ASTString> name = expectIdentifierToken();
	vector<ASTPointer<EnumValue>> members;
	expectToken(Token::LBrace);

	while (m_scanner->currentToken() != Token::RBrace)
	{
		members.push_back(parseEnumValue());
		if (m_scanner->currentToken() == Token::RBrace)
			break;
		expectToken(Token::Comma);
		if (m_scanner->currentToken() != Token::Identifier)
			fatalParserError(std::string("Expected Identifier after ','"));
	}

	nodeFactory.markEndPosition();
	expectToken(Token::RBrace);
	return nodeFactory.createNode<EnumDefinition>(name, members);
}

ASTPointer<VariableDeclaration> Parser::parseVariableDeclaration(
	VarDeclParserOptions const& _options,
	ASTPointer<TypeName> const& _lookAheadArrayType
)
{
	ASTNodeFactory nodeFactory = _lookAheadArrayType ?
		ASTNodeFactory(*this, _lookAheadArrayType) : ASTNodeFactory(*this);
	ASTPointer<TypeName> type;
	if (_lookAheadArrayType)
		type = _lookAheadArrayType;
	else
	{
		type = parseTypeName(_options.allowVar);
		if (type != nullptr)
			nodeFactory.setEndPositionFromNode(type);
	}
	bool isIndexed = false;
	bool isDeclaredConst = false;
	Declaration::Visibility visibility(Declaration::Visibility::Default);
	VariableDeclaration::Location location = VariableDeclaration::Location::Default;
	ASTPointer<ASTString> identifier;

	while (true)
	{
		Token::Value token = m_scanner->currentToken();
		if (_options.isStateVariable && Token::isVariableVisibilitySpecifier(token))
		{
			if (visibility != Declaration::Visibility::Default)
				fatalParserError(std::string("Visibility already specified."));
			visibility = parseVisibilitySpecifier(token);
		}
		else
		{
			if (_options.allowIndexed && token == Token::Indexed)
				isIndexed = true;
			else if (token == Token::Const)
				isDeclaredConst = true;
			else if (_options.allowLocationSpecifier && Token::isLocationSpecifier(token))
			{
				if (location != VariableDeclaration::Location::Default)
					fatalParserError(std::string("Location already specified."));
				if (!type)
					fatalParserError(std::string("Location specifier needs explicit type name."));
				location = (
					token == Token::Memory ?
					VariableDeclaration::Location::Memory :
					VariableDeclaration::Location::Storage
				);
			}
			else
				break;
			m_scanner->next();
		}
	}
	nodeFactory.markEndPosition();

	if (_options.allowEmptyName && m_scanner->currentToken() != Token::Identifier)
	{
		identifier = make_shared<ASTString>("");
		solAssert(type != nullptr, "");
		nodeFactory.setEndPositionFromNode(type);
	}
	else
		identifier = expectIdentifierToken();
	ASTPointer<Expression> value;
	if (_options.allowInitialValue)
	{
		if (m_scanner->currentToken() == Token::Assign)
		{
			m_scanner->next();
			value = parseExpression();
			nodeFactory.setEndPositionFromNode(value);
		}
	}
	return nodeFactory.createNode<VariableDeclaration>(
		type,
		identifier,
		value,
		visibility,
		_options.isStateVariable,
		isIndexed,
		isDeclaredConst,
		location
	);
}

ASTPointer<ModifierDefinition> Parser::parseModifierDefinition()
{
	ScopeGuard resetModifierFlag([this]() { m_insideModifier = false; });
	m_insideModifier = true;

	ASTNodeFactory nodeFactory(*this);
	ASTPointer<ASTString> docstring;
	if (m_scanner->currentCommentLiteral() != "")
		docstring = make_shared<ASTString>(m_scanner->currentCommentLiteral());

	expectToken(Token::Modifier);
	ASTPointer<ASTString> name(expectIdentifierToken());
	ASTPointer<ParameterList> parameters;
	if (m_scanner->currentToken() == Token::LParen)
	{
		VarDeclParserOptions options;
		options.allowIndexed = true;
		options.allowLocationSpecifier = true;
		parameters = parseParameterList(options);
	}
	else
		parameters = createEmptyParameterList();
	ASTPointer<Block> block = parseBlock();
	nodeFactory.setEndPositionFromNode(block);
	return nodeFactory.createNode<ModifierDefinition>(name, docstring, parameters, block);
}

ASTPointer<EventDefinition> Parser::parseEventDefinition()
{
	ASTNodeFactory nodeFactory(*this);
	ASTPointer<ASTString> docstring;
	if (m_scanner->currentCommentLiteral() != "")
		docstring = make_shared<ASTString>(m_scanner->currentCommentLiteral());

	expectToken(Token::Event);
	ASTPointer<ASTString> name(expectIdentifierToken());
	ASTPointer<ParameterList> parameters;
	if (m_scanner->currentToken() == Token::LParen)
	{
		VarDeclParserOptions options;
		options.allowIndexed = true;
		parameters = parseParameterList(options);
	}
	else
		parameters = createEmptyParameterList();
	bool anonymous = false;
	if (m_scanner->currentToken() == Token::Anonymous)
	{
		anonymous = true;
		m_scanner->next();
	}
	nodeFactory.markEndPosition();
	expectToken(Token::Semicolon);
	return nodeFactory.createNode<EventDefinition>(name, docstring, parameters, anonymous);
}

ASTPointer<UsingForDirective> Parser::parseUsingDirective()
{
	ASTNodeFactory nodeFactory(*this);

	expectToken(Token::Using);
	//@todo this should actually parse a full path.
	ASTPointer<Identifier> library(parseIdentifier());
	ASTPointer<TypeName> typeName;
	expectToken(Token::For);
	if (m_scanner->currentToken() == Token::Mul)
		m_scanner->next();
	else
		typeName = parseTypeName(false);
	nodeFactory.markEndPosition();
	expectToken(Token::Semicolon);
	return nodeFactory.createNode<UsingForDirective>(library, typeName);
}

ASTPointer<ModifierInvocation> Parser::parseModifierInvocation()
{
	ASTNodeFactory nodeFactory(*this);
	ASTPointer<Identifier> name(parseIdentifier());
	vector<ASTPointer<Expression>> arguments;
	if (m_scanner->currentToken() == Token::LParen)
	{
		m_scanner->next();
		arguments = parseFunctionCallListArguments();
		nodeFactory.markEndPosition();
		expectToken(Token::RParen);
	}
	else
		nodeFactory.setEndPositionFromNode(name);
	return nodeFactory.createNode<ModifierInvocation>(name, arguments);
}

ASTPointer<Identifier> Parser::parseIdentifier()
{
	ASTNodeFactory nodeFactory(*this);
	nodeFactory.markEndPosition();
	return nodeFactory.createNode<Identifier>(expectIdentifierToken());
}

ASTPointer<TypeName> Parser::parseTypeName(bool _allowVar)
{
	ASTNodeFactory nodeFactory(*this);
	ASTPointer<TypeName> type;
	Token::Value token = m_scanner->currentToken();
	if (Token::isElementaryTypeName(token))
	{
		type = ASTNodeFactory(*this).createNode<ElementaryTypeName>(token);
		m_scanner->next();
	}
	else if (token == Token::Var)
	{
		if (!_allowVar)
			fatalParserError(std::string("Expected explicit type name."));
		m_scanner->next();
	}
	else if (token == Token::Mapping)
		type = parseMapping();
	else if (token == Token::Identifier)
	{
		ASTNodeFactory nodeFactory(*this);
		nodeFactory.markEndPosition();
		vector<ASTString> identifierPath{*expectIdentifierToken()};
		while (m_scanner->currentToken() == Token::Period)
		{
			m_scanner->next();
			nodeFactory.markEndPosition();
			identifierPath.push_back(*expectIdentifierToken());
		}
		type = nodeFactory.createNode<UserDefinedTypeName>(identifierPath);
	}
	else
		fatalParserError(std::string("Expected type name"));

	if (type)
		// Parse "[...]" postfixes for arrays.
		while (m_scanner->currentToken() == Token::LBrack)
		{
			m_scanner->next();
			ASTPointer<Expression> length;
			if (m_scanner->currentToken() != Token::RBrack)
				length = parseExpression();
			nodeFactory.markEndPosition();
			expectToken(Token::RBrack);
			type = nodeFactory.createNode<ArrayTypeName>(type, length);
		}
	return type;
}

ASTPointer<Mapping> Parser::parseMapping()
{
	ASTNodeFactory nodeFactory(*this);
	expectToken(Token::Mapping);
	expectToken(Token::LParen);
	if (!Token::isElementaryTypeName(m_scanner->currentToken()))
		fatalParserError(std::string("Expected elementary type name for mapping key type"));
	ASTPointer<ElementaryTypeName> keyType;
	keyType = ASTNodeFactory(*this).createNode<ElementaryTypeName>(m_scanner->currentToken());
	m_scanner->next();
	expectToken(Token::Arrow);
	bool const allowVar = false;
	ASTPointer<TypeName> valueType = parseTypeName(allowVar);
	nodeFactory.markEndPosition();
	expectToken(Token::RParen);
	return nodeFactory.createNode<Mapping>(keyType, valueType);
}

ASTPointer<ParameterList> Parser::parseParameterList(
	VarDeclParserOptions const& _options,
	bool _allowEmpty
)
{
	ASTNodeFactory nodeFactory(*this);
	vector<ASTPointer<VariableDeclaration>> parameters;
	VarDeclParserOptions options(_options);
	options.allowEmptyName = true;
	expectToken(Token::LParen);
	if (!_allowEmpty || m_scanner->currentToken() != Token::RParen)
	{
		parameters.push_back(parseVariableDeclaration(options));
		while (m_scanner->currentToken() != Token::RParen)
		{
			expectToken(Token::Comma);
			parameters.push_back(parseVariableDeclaration(options));
		}
	}
	nodeFactory.markEndPosition();
	m_scanner->next();
	return nodeFactory.createNode<ParameterList>(parameters);
}

ASTPointer<Block> Parser::parseBlock(ASTPointer<ASTString> const& _docString)
{
	ASTNodeFactory nodeFactory(*this);
	expectToken(Token::LBrace);
	vector<ASTPointer<Statement>> statements;
	while (m_scanner->currentToken() != Token::RBrace)
		statements.push_back(parseStatement());
	nodeFactory.markEndPosition();
	expectToken(Token::RBrace);
	return nodeFactory.createNode<Block>(_docString, statements);
}

ASTPointer<Statement> Parser::parseStatement()
{
	ASTPointer<ASTString> docString;
	if (m_scanner->currentCommentLiteral() != "")
		docString = make_shared<ASTString>(m_scanner->currentCommentLiteral());
	ASTPointer<Statement> statement;
	switch (m_scanner->currentToken())
	{
	case Token::If:
		return parseIfStatement(docString);
	case Token::While:
		return parseWhileStatement(docString);
	case Token::For:
		return parseForStatement(docString);
	case Token::LBrace:
		return parseBlock(docString);
		// starting from here, all statements must be terminated by a semicolon
	case Token::Continue:
		statement = ASTNodeFactory(*this).createNode<Continue>(docString);
		m_scanner->next();
		break;
	case Token::Break:
		statement = ASTNodeFactory(*this).createNode<Break>(docString);
		m_scanner->next();
		break;
	case Token::Return:
	{
		ASTNodeFactory nodeFactory(*this);
		ASTPointer<Expression> expression;
		if (m_scanner->next() != Token::Semicolon)
		{
			expression = parseExpression();
			nodeFactory.setEndPositionFromNode(expression);
		}
		statement = nodeFactory.createNode<Return>(docString, expression);
		break;
	}
	case Token::Throw:
	{
		statement = ASTNodeFactory(*this).createNode<Throw>(docString);
		m_scanner->next();
		break;
	}
	case Token::Identifier:
		if (m_insideModifier && m_scanner->currentLiteral() == "_")
		{
			statement = ASTNodeFactory(*this).createNode<PlaceholderStatement>(docString);
			m_scanner->next();
			return statement;
		}
	// fall-through
	default:
		statement = parseSimpleStatement(docString);
	}
	expectToken(Token::Semicolon);
	return statement;
}

ASTPointer<IfStatement> Parser::parseIfStatement(ASTPointer<ASTString> const& _docString)
{
	ASTNodeFactory nodeFactory(*this);
	expectToken(Token::If);
	expectToken(Token::LParen);
	ASTPointer<Expression> condition = parseExpression();
	expectToken(Token::RParen);
	ASTPointer<Statement> trueBody = parseStatement();
	ASTPointer<Statement> falseBody;
	if (m_scanner->currentToken() == Token::Else)
	{
		m_scanner->next();
		falseBody = parseStatement();
		nodeFactory.setEndPositionFromNode(falseBody);
	}
	else
		nodeFactory.setEndPositionFromNode(trueBody);
	return nodeFactory.createNode<IfStatement>(_docString, condition, trueBody, falseBody);
}

ASTPointer<WhileStatement> Parser::parseWhileStatement(ASTPointer<ASTString> const& _docString)
{
	ASTNodeFactory nodeFactory(*this);
	expectToken(Token::While);
	expectToken(Token::LParen);
	ASTPointer<Expression> condition = parseExpression();
	expectToken(Token::RParen);
	ASTPointer<Statement> body = parseStatement();
	nodeFactory.setEndPositionFromNode(body);
	return nodeFactory.createNode<WhileStatement>(_docString, condition, body);
}

ASTPointer<ForStatement> Parser::parseForStatement(ASTPointer<ASTString> const& _docString)
{
	ASTNodeFactory nodeFactory(*this);
	ASTPointer<Statement> initExpression;
	ASTPointer<Expression> conditionExpression;
	ASTPointer<ExpressionStatement> loopExpression;
	expectToken(Token::For);
	expectToken(Token::LParen);

	// LTODO: Maybe here have some predicate like peekExpression() instead of checking for semicolon and RParen?
	if (m_scanner->currentToken() != Token::Semicolon)
		initExpression = parseSimpleStatement(ASTPointer<ASTString>());
	expectToken(Token::Semicolon);

	if (m_scanner->currentToken() != Token::Semicolon)
		conditionExpression = parseExpression();
	expectToken(Token::Semicolon);

	if (m_scanner->currentToken() != Token::RParen)
		loopExpression = parseExpressionStatement(ASTPointer<ASTString>());
	expectToken(Token::RParen);

	ASTPointer<Statement> body = parseStatement();
	nodeFactory.setEndPositionFromNode(body);
	return nodeFactory.createNode<ForStatement>(
		_docString,
		initExpression,
		conditionExpression,
		loopExpression,
		body
	);
}

ASTPointer<Statement> Parser::parseSimpleStatement(ASTPointer<ASTString> const& _docString)
{
	// These two cases are very hard to distinguish:
	// x[7 * 20 + 3] a;  -  x[7 * 20 + 3] = 9;
	// In the first case, x is a type name, in the second it is the name of a variable.
	// As an extension, we can even have:
	// `x.y.z[1][2] a;` and `x.y.z[1][2] = 10;`
	// Where in the first, x.y.z leads to a type name where in the second, it accesses structs.
	switch (peekStatementType())
	{
	case LookAheadInfo::VariableDeclarationStatement:
		return parseVariableDeclarationStatement(_docString);
	case LookAheadInfo::ExpressionStatement:
		return parseExpressionStatement(_docString);
	default:
		break;
	}

	// At this point, we have 'Identifier "["' or 'Identifier "." Identifier' or 'ElementoryTypeName "["'.
	// We parse '(Identifier ("." Identifier)* |ElementaryTypeName) ( "[" Expression "]" )+'
	// until we can decide whether to hand this over to ExpressionStatement or create a
	// VariableDeclarationStatement out of it.

	vector<ASTPointer<PrimaryExpression>> path;
	bool startedWithElementary = false;
	if (m_scanner->currentToken() == Token::Identifier)
		path.push_back(parseIdentifier());
	else
	{
		startedWithElementary = true;
		path.push_back(ASTNodeFactory(*this).createNode<ElementaryTypeNameExpression>(m_scanner->currentToken()));
		m_scanner->next();
	}
	while (!startedWithElementary && m_scanner->currentToken() == Token::Period)
	{
		m_scanner->next();
		path.push_back(parseIdentifier());
	}
	vector<pair<ASTPointer<Expression>, SourceLocation>> indices;
	while (m_scanner->currentToken() == Token::LBrack)
	{
		expectToken(Token::LBrack);
		ASTPointer<Expression> index;
		if (m_scanner->currentToken() != Token::RBrack)
			index = parseExpression();
		SourceLocation indexLocation = path.front()->location();
		indexLocation.end = endPosition();
		indices.push_back(make_pair(index, indexLocation));
		expectToken(Token::RBrack);
	}

	if (m_scanner->currentToken() == Token::Identifier || Token::isLocationSpecifier(m_scanner->currentToken()))
		return parseVariableDeclarationStatement(_docString, typeNameIndexAccessStructure(path, indices));
	else
		return parseExpressionStatement(_docString, expressionFromIndexAccessStructure(path, indices));
}

ASTPointer<VariableDeclarationStatement> Parser::parseVariableDeclarationStatement(
	ASTPointer<ASTString> const& _docString,
	ASTPointer<TypeName> const& _lookAheadArrayType
)
{
	ASTNodeFactory nodeFactory(*this);
	if (_lookAheadArrayType)
		nodeFactory.setLocation(_lookAheadArrayType->location());
	vector<ASTPointer<VariableDeclaration>> variables;
	ASTPointer<Expression> value;
	if (
		!_lookAheadArrayType &&
		m_scanner->currentToken() == Token::Var &&
		m_scanner->peekNextToken() == Token::LParen
	)
	{
		// Parse `var (a, b, ,, c) = ...` into a single VariableDeclarationStatement with multiple variables.
		m_scanner->next();
		m_scanner->next();
		if (m_scanner->currentToken() != Token::RParen)
			while (true)
			{
				ASTPointer<VariableDeclaration> var;
				if (
					m_scanner->currentToken() != Token::Comma &&
					m_scanner->currentToken() != Token::RParen
				)
				{
					ASTNodeFactory varDeclNodeFactory(*this);
					varDeclNodeFactory.markEndPosition();
					ASTPointer<ASTString> name = expectIdentifierToken();
					var = varDeclNodeFactory.createNode<VariableDeclaration>(
						ASTPointer<TypeName>(),
						name,
						ASTPointer<Expression>(),
						VariableDeclaration::Visibility::Default
					);
				}
				variables.push_back(var);
				if (m_scanner->currentToken() == Token::RParen)
					break;
				else
					expectToken(Token::Comma);
			}
		nodeFactory.markEndPosition();
		m_scanner->next();
	}
	else
	{
		VarDeclParserOptions options;
		options.allowVar = true;
		options.allowLocationSpecifier = true;
		variables.push_back(parseVariableDeclaration(options, _lookAheadArrayType));
	}
	if (m_scanner->currentToken() == Token::Assign)
	{
		m_scanner->next();
		value = parseExpression();
		nodeFactory.setEndPositionFromNode(value);
	}
	return nodeFactory.createNode<VariableDeclarationStatement>(_docString, variables, value);
}

ASTPointer<ExpressionStatement> Parser::parseExpressionStatement(
	ASTPointer<ASTString> const& _docString,
	ASTPointer<Expression> const& _lookAheadIndexAccessStructure
)
{
	ASTPointer<Expression> expression = parseExpression(_lookAheadIndexAccessStructure);
	return ASTNodeFactory(*this, expression).createNode<ExpressionStatement>(_docString, expression);
}

ASTPointer<Expression> Parser::parseExpression(
	ASTPointer<Expression> const& _lookAheadIndexAccessStructure
)
{
	ASTPointer<Expression> expression = parseBinaryExpression(4, _lookAheadIndexAccessStructure);
	if (!Token::isAssignmentOp(m_scanner->currentToken()))
		return expression;
	Token::Value assignmentOperator = expectAssignmentOperator();
	ASTPointer<Expression> rightHandSide = parseExpression();
	ASTNodeFactory nodeFactory(*this, expression);
	nodeFactory.setEndPositionFromNode(rightHandSide);
	return nodeFactory.createNode<Assignment>(expression, assignmentOperator, rightHandSide);
}

ASTPointer<Expression> Parser::parseBinaryExpression(
	int _minPrecedence,
	ASTPointer<Expression> const& _lookAheadIndexAccessStructure
)
{
	ASTPointer<Expression> expression = parseUnaryExpression(_lookAheadIndexAccessStructure);
	ASTNodeFactory nodeFactory(*this, expression);
	int precedence = Token::precedence(m_scanner->currentToken());
	for (; precedence >= _minPrecedence; --precedence)
		while (Token::precedence(m_scanner->currentToken()) == precedence)
		{
			Token::Value op = m_scanner->currentToken();
			m_scanner->next();
			ASTPointer<Expression> right = parseBinaryExpression(precedence + 1);
			nodeFactory.setEndPositionFromNode(right);
			expression = nodeFactory.createNode<BinaryOperation>(expression, op, right);
		}
	return expression;
}

ASTPointer<Expression> Parser::parseUnaryExpression(
	ASTPointer<Expression> const& _lookAheadIndexAccessStructure
)
{
	ASTNodeFactory nodeFactory = _lookAheadIndexAccessStructure ?
		ASTNodeFactory(*this, _lookAheadIndexAccessStructure) : ASTNodeFactory(*this);
	Token::Value token = m_scanner->currentToken();
	if (!_lookAheadIndexAccessStructure && (Token::isUnaryOp(token) || Token::isCountOp(token)))
	{
		// prefix expression
		m_scanner->next();
		ASTPointer<Expression> subExpression = parseUnaryExpression();
		nodeFactory.setEndPositionFromNode(subExpression);
		return nodeFactory.createNode<UnaryOperation>(token, subExpression, true);
	}
	else
	{
		// potential postfix expression
		ASTPointer<Expression> subExpression = parseLeftHandSideExpression(_lookAheadIndexAccessStructure);
		token = m_scanner->currentToken();
		if (!Token::isCountOp(token))
			return subExpression;
		nodeFactory.markEndPosition();
		m_scanner->next();
		return nodeFactory.createNode<UnaryOperation>(token, subExpression, false);
	}
}

ASTPointer<Expression> Parser::parseLeftHandSideExpression(
	ASTPointer<Expression> const& _lookAheadIndexAccessStructure
)
{
	ASTNodeFactory nodeFactory = _lookAheadIndexAccessStructure ?
		ASTNodeFactory(*this, _lookAheadIndexAccessStructure) : ASTNodeFactory(*this);

	ASTPointer<Expression> expression;
	if (_lookAheadIndexAccessStructure)
		expression = _lookAheadIndexAccessStructure;
	else if (m_scanner->currentToken() == Token::New)
	{
		expectToken(Token::New);
		ASTPointer<TypeName> contractName(parseTypeName(false));
		nodeFactory.setEndPositionFromNode(contractName);
		expression = nodeFactory.createNode<NewExpression>(contractName);
	}
	else
		expression = parsePrimaryExpression();

	while (true)
	{
		switch (m_scanner->currentToken())
		{
		case Token::LBrack:
		{
			m_scanner->next();
			ASTPointer<Expression> index;
			if (m_scanner->currentToken() != Token::RBrack)
				index = parseExpression();
			nodeFactory.markEndPosition();
			expectToken(Token::RBrack);
			expression = nodeFactory.createNode<IndexAccess>(expression, index);
		}
		break;
		case Token::Period:
		{
			m_scanner->next();
			nodeFactory.markEndPosition();
			expression = nodeFactory.createNode<MemberAccess>(expression, expectIdentifierToken());
		}
		break;
		case Token::LParen:
		{
			m_scanner->next();
			vector<ASTPointer<Expression>> arguments;
			vector<ASTPointer<ASTString>> names;
			std::tie(arguments, names) = parseFunctionCallArguments();
			nodeFactory.markEndPosition();
			expectToken(Token::RParen);
			expression = nodeFactory.createNode<FunctionCall>(expression, arguments, names);
		}
		break;
		default:
			return expression;
		}
	}
}

ASTPointer<Expression> Parser::parsePrimaryExpression()
{
	ASTNodeFactory nodeFactory(*this);
	Token::Value token = m_scanner->currentToken();
	ASTPointer<Expression> expression;
	switch (token)
	{
	case Token::TrueLiteral:
	case Token::FalseLiteral:
		expression = nodeFactory.createNode<Literal>(token, getLiteralAndAdvance());
		break;
	case Token::Number:
		if (Token::isEtherSubdenomination(m_scanner->peekNextToken()))
		{
			ASTPointer<ASTString> literal = getLiteralAndAdvance();
			nodeFactory.markEndPosition();
			Literal::SubDenomination subdenomination = static_cast<Literal::SubDenomination>(m_scanner->currentToken());
			m_scanner->next();
			expression = nodeFactory.createNode<Literal>(token, literal, subdenomination);
			break;
		}
		if (Token::isTimeSubdenomination(m_scanner->peekNextToken()))
		{
			ASTPointer<ASTString> literal = getLiteralAndAdvance();
			nodeFactory.markEndPosition();
			Literal::SubDenomination subdenomination = static_cast<Literal::SubDenomination>(m_scanner->currentToken());
			m_scanner->next();
			expression = nodeFactory.createNode<Literal>(token, literal, subdenomination);
			break;
		}
		// fall-through
	case Token::StringLiteral:
		nodeFactory.markEndPosition();
		expression = nodeFactory.createNode<Literal>(token, getLiteralAndAdvance());
		break;
	case Token::Identifier:
		nodeFactory.markEndPosition();
		expression = nodeFactory.createNode<Identifier>(getLiteralAndAdvance());
		break;
	case Token::LParen:
	case Token::LBrack:
	{
		// Tuple/parenthesized expression or inline array/bracketed expression.
		// Special cases: ()/[] is empty tuple/array type, (x)/[] is not a real tuple/array,
		// (x,) is one-dimensional tuple
		m_scanner->next();
		vector<ASTPointer<Expression>> components;
		Token::Value oppositeToken = (token == Token::LParen ? Token::RParen : Token::RBrack);
		bool isArray = (token == Token::LBrack ? true : false);
		if (isArray && (m_scanner->currentToken() == Token::Comma))
			fatalParserError(std::string("Expected value in array cell after '[' ."));
		if (m_scanner->currentToken() != oppositeToken)
			while (true)
			{
				if (m_scanner->currentToken() != Token::Comma && m_scanner->currentToken() != oppositeToken)
					components.push_back(parseExpression());
				else
					components.push_back(ASTPointer<Expression>());
				if (m_scanner->currentToken() == oppositeToken)
					break;
				else if (m_scanner->currentToken() == Token::Comma)
					if (isArray && (m_scanner->peekNextToken() == (Token::Comma || oppositeToken)))
						fatalParserError(std::string("Expected value in array cell after ',' ."));
					m_scanner->next();
			}
		nodeFactory.markEndPosition();
		expectToken(oppositeToken);
		return nodeFactory.createNode<TupleExpression>(components, isArray);
	}

	default:
		if (Token::isElementaryTypeName(token))
		{
			// used for casts
			expression = nodeFactory.createNode<ElementaryTypeNameExpression>(token);
			m_scanner->next();
		}
		else
			fatalParserError(std::string("Expected primary expression."));
		break;
	}
	return expression;
}

vector<ASTPointer<Expression>> Parser::parseFunctionCallListArguments()
{
	vector<ASTPointer<Expression>> arguments;
	if (m_scanner->currentToken() != Token::RParen)
	{
		arguments.push_back(parseExpression());
		while (m_scanner->currentToken() != Token::RParen)
		{
			expectToken(Token::Comma);
			arguments.push_back(parseExpression());
		}
	}
	return arguments;
}

pair<vector<ASTPointer<Expression>>, vector<ASTPointer<ASTString>>> Parser::parseFunctionCallArguments()
{
	pair<vector<ASTPointer<Expression>>, vector<ASTPointer<ASTString>>> ret;
	Token::Value token = m_scanner->currentToken();
	if (token == Token::LBrace)
	{
		// call({arg1 : 1, arg2 : 2 })
		expectToken(Token::LBrace);
		while (m_scanner->currentToken() != Token::RBrace)
		{
			ret.second.push_back(expectIdentifierToken());
			expectToken(Token::Colon);
			ret.first.push_back(parseExpression());

			if (m_scanner->currentToken() == Token::Comma)
				expectToken(Token::Comma);
			else
				break;
		}
		expectToken(Token::RBrace);
	}
	else
		ret.first = parseFunctionCallListArguments();
	return ret;
}

Parser::LookAheadInfo Parser::peekStatementType() const
{
	// Distinguish between variable declaration (and potentially assignment) and expression statement
	// (which include assignments to other expressions and pre-declared variables).
	// We have a variable declaration if we get a keyword that specifies a type name.
	// If it is an identifier or an elementary type name followed by an identifier, we also have
	// a variable declaration.
	// If we get an identifier followed by a "[" or ".", it can be both ("lib.type[9] a;" or "variable.el[9] = 7;").
	// In all other cases, we have an expression statement.
	Token::Value token(m_scanner->currentToken());
	bool mightBeTypeName = (Token::isElementaryTypeName(token) || token == Token::Identifier);

	if (token == Token::Mapping || token == Token::Var)
		return LookAheadInfo::VariableDeclarationStatement;
	if (mightBeTypeName)
	{
		Token::Value next = m_scanner->peekNextToken();
		if (next == Token::Identifier || Token::isLocationSpecifier(next))
			return LookAheadInfo::VariableDeclarationStatement;
		if (next == Token::LBrack || next == Token::Period)
			return LookAheadInfo::IndexAccessStructure;
	}
	return LookAheadInfo::ExpressionStatement;
}

ASTPointer<TypeName> Parser::typeNameIndexAccessStructure(
	vector<ASTPointer<PrimaryExpression>> const& _path,
	vector<pair<ASTPointer<Expression>, SourceLocation>> const& _indices
)
{
	solAssert(!_path.empty(), "");
	ASTNodeFactory nodeFactory(*this);
	SourceLocation location = _path.front()->location();
	location.end = _path.back()->location().end;
	nodeFactory.setLocation(location);

	ASTPointer<TypeName> type;
	if (auto typeName = dynamic_cast<ElementaryTypeNameExpression const*>(_path.front().get()))
	{
		solAssert(_path.size() == 1, "");
		type = nodeFactory.createNode<ElementaryTypeName>(typeName->typeToken());
	}
	else
	{
		vector<ASTString> path;
		for (auto const& el: _path)
			path.push_back(dynamic_cast<Identifier const&>(*el).name());
		type = nodeFactory.createNode<UserDefinedTypeName>(path);
	}
	for (auto const& lengthExpression: _indices)
	{
		nodeFactory.setLocation(lengthExpression.second);
		type = nodeFactory.createNode<ArrayTypeName>(type, lengthExpression.first);
	}
	return type;
}

ASTPointer<Expression> Parser::expressionFromIndexAccessStructure(
	vector<ASTPointer<PrimaryExpression>> const& _path,
	vector<pair<ASTPointer<Expression>, SourceLocation>> const& _indices
)
{
	solAssert(!_path.empty(), "");
	ASTNodeFactory nodeFactory(*this, _path.front());
	ASTPointer<Expression> expression(_path.front());
	for (size_t i = 1; i < _path.size(); ++i)
	{
		SourceLocation location(_path.front()->location());
		location.end = _path[i]->location().end;
		nodeFactory.setLocation(location);
		Identifier const& identifier = dynamic_cast<Identifier const&>(*_path[i]);
		expression = nodeFactory.createNode<MemberAccess>(
			expression,
			make_shared<ASTString>(identifier.name())
		);
	}
	for (auto const& index: _indices)
	{
		nodeFactory.setLocation(index.second);
		expression = nodeFactory.createNode<IndexAccess>(expression, index.first);
	}
	return expression;
}

void Parser::expectToken(Token::Value _value)
{
	if (m_scanner->currentToken() != _value)
		fatalParserError(
			string("Expected token ") +
			string(Token::name(_value)) +
			string(" got '") +
			string(Token::name(m_scanner->currentToken())) +
			string("'")
		);
	m_scanner->next();
}

Token::Value Parser::expectAssignmentOperator()
{
	Token::Value op = m_scanner->currentToken();
	if (!Token::isAssignmentOp(op))
		fatalParserError(
			std::string("Expected assignment operator ") +
			string(" got '") +
			string(Token::name(m_scanner->currentToken())) +
			string("'")
		);
	m_scanner->next();
	return op;
}

ASTPointer<ASTString> Parser::expectIdentifierToken()
{
	if (m_scanner->currentToken() != Token::Identifier)
		fatalParserError(
			std::string("Expected identifier ") +
			string(" got '") +
			string(Token::name(m_scanner->currentToken())) +
			string("'")
		);
	return getLiteralAndAdvance();
}

ASTPointer<ASTString> Parser::getLiteralAndAdvance()
{
	ASTPointer<ASTString> identifier = make_shared<ASTString>(m_scanner->currentLiteral());
	m_scanner->next();
	return identifier;
}

ASTPointer<ParameterList> Parser::createEmptyParameterList()
{
	ASTNodeFactory nodeFactory(*this);
	nodeFactory.setLocationEmpty();
	return nodeFactory.createNode<ParameterList>(vector<ASTPointer<VariableDeclaration>>());
}

void Parser::parserError(string const& _description)
{
	auto err = make_shared<Error>(Error::Type::ParserError);
	*err <<
		errinfo_sourceLocation(SourceLocation(position(), position(), sourceName())) <<
		errinfo_comment(_description);

	m_errors.push_back(err);
}

void Parser::fatalParserError(string const& _description)
{
	parserError(_description);
	BOOST_THROW_EXCEPTION(FatalError());
}

}
}
