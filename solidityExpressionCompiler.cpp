
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
 * Unit tests for the solidity expression compiler.
 */

#include <string>

#include <libdevcore/Log.h>
#include <libsolidity/Scanner.h>
#include <libsolidity/Parser.h>
#include <libsolidity/NameAndTypeResolver.h>
#include <libsolidity/CompilerContext.h>
#include <libsolidity/ExpressionCompiler.h>
#include <libsolidity/AST.h>
#include <boost/test/unit_test.hpp>

using namespace std;

namespace dev
{
namespace solidity
{
namespace test
{

namespace
{

/// Helper class that extracts the first expression in an AST.
class FirstExpressionExtractor: private ASTVisitor
{
public:
	FirstExpressionExtractor(ASTNode& _node): m_expression(nullptr) { _node.accept(*this); }
	Expression* getExpression() const { return m_expression; }
private:
	virtual bool visit(Expression& _expression) override { return checkExpression(_expression); }
	virtual bool visit(Assignment& _expression) override { return checkExpression(_expression); }
	virtual bool visit(UnaryOperation& _expression) override { return checkExpression(_expression); }
	virtual bool visit(BinaryOperation& _expression) override { return checkExpression(_expression); }
	virtual bool visit(FunctionCall& _expression) override { return checkExpression(_expression); }
	virtual bool visit(MemberAccess& _expression) override { return checkExpression(_expression); }
	virtual bool visit(IndexAccess& _expression) override { return checkExpression(_expression); }
	virtual bool visit(PrimaryExpression& _expression) override { return checkExpression(_expression); }
	virtual bool visit(Identifier& _expression) override { return checkExpression(_expression); }
	virtual bool visit(ElementaryTypeNameExpression& _expression) override { return checkExpression(_expression); }
	virtual bool visit(Literal& _expression) override { return checkExpression(_expression); }
	bool checkExpression(Expression& _expression)
	{
		if (m_expression == nullptr)
			m_expression = &_expression;
		return false;
	}
private:
	Expression* m_expression;
};

Declaration const& resolveDeclaration(vector<string> const& _namespacedName,
											  NameAndTypeResolver const& _resolver)
{
	Declaration const* declaration = nullptr;
	// bracers are required, cause msvc couldnt handle this macro in for statement
	for (string const& namePart: _namespacedName)
	{
		BOOST_REQUIRE(declaration = _resolver.resolveName(namePart, declaration));
	}
	BOOST_REQUIRE(declaration);
	return *declaration;
}

bytes compileFirstExpression(const string& _sourceCode, vector<vector<string>> _functions = {},
							 vector<vector<string>> _localVariables = {})
{
	Parser parser;
	ASTPointer<SourceUnit> sourceUnit;
	BOOST_REQUIRE_NO_THROW(sourceUnit = parser.parse(make_shared<Scanner>(CharStream(_sourceCode))));
	NameAndTypeResolver resolver({});
	resolver.registerDeclarations(*sourceUnit);
	for (ASTPointer<ASTNode> const& node: sourceUnit->getNodes())
		if (ContractDefinition* contract = dynamic_cast<ContractDefinition*>(node.get()))
			BOOST_REQUIRE_NO_THROW(resolver.resolveNamesAndTypes(*contract));
	for (ASTPointer<ASTNode> const& node: sourceUnit->getNodes())
		if (ContractDefinition* contract = dynamic_cast<ContractDefinition*>(node.get()))
			BOOST_REQUIRE_NO_THROW(resolver.checkTypeRequirements(*contract));
	for (ASTPointer<ASTNode> const& node: sourceUnit->getNodes())
		if (ContractDefinition* contract = dynamic_cast<ContractDefinition*>(node.get()))
		{
			FirstExpressionExtractor extractor(*contract);
			BOOST_REQUIRE(extractor.getExpression() != nullptr);

			CompilerContext context;
			for (vector<string> const& function: _functions)
				context.addFunction(dynamic_cast<FunctionDefinition const&>(resolveDeclaration(function, resolver)));
			for (vector<string> const& variable: _localVariables)
				context.addVariable(dynamic_cast<VariableDeclaration const&>(resolveDeclaration(variable, resolver)));

			ExpressionCompiler::compileExpression(context, *extractor.getExpression());

			for (vector<string> const& function: _functions)
				context << context.getFunctionEntryLabel(dynamic_cast<FunctionDefinition const&>(resolveDeclaration(function, resolver)));
			bytes instructions = context.getAssembledBytecode();
			// debug
			// cout << eth::disassemble(instructions) << endl;
			return instructions;
		}
	BOOST_FAIL("No contract found in source.");
	return bytes();
}

} // end anonymous namespace

BOOST_AUTO_TEST_SUITE(SolidityExpressionCompiler)

BOOST_AUTO_TEST_CASE(literal_true)
{
	char const* sourceCode = "contract test {\n"
							 "  function f() { var x = true; }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({byte(eth::Instruction::PUSH1), 0x1});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(literal_false)
{
	char const* sourceCode = "contract test {\n"
							 "  function f() { var x = false; }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({byte(eth::Instruction::PUSH1), 0x0});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(int_literal)
{
	char const* sourceCode = "contract test {\n"
							 "  function f() { var x = 0x12345678901234567890; }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({byte(eth::Instruction::PUSH10), 0x12, 0x34, 0x56, 0x78, 0x90,
													   0x12, 0x34, 0x56, 0x78, 0x90});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(comparison)
{
	char const* sourceCode = "contract test {\n"
							 "  function f() { var x = (0x10aa < 0x11aa) != true; }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({byte(eth::Instruction::PUSH1), 0x1,
					   byte(eth::Instruction::PUSH2), 0x11, 0xaa, byte(eth::Instruction::PUSH2), 0xff, 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::PUSH2), 0x10, 0xaa, byte(eth::Instruction::PUSH2), 0xff, 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::LT),
					   byte(eth::Instruction::EQ),
					   byte(eth::Instruction::ISZERO)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(short_circuiting)
{
	char const* sourceCode = "contract test {\n"
							 "  function f() { var x = true != (4 <= 8 + 10 || 9 != 2); }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({byte(eth::Instruction::PUSH1), 0xa,
					   byte(eth::Instruction::PUSH1), 0x8,
					   byte(eth::Instruction::ADD), byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::PUSH1), 0x4, byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::GT),
					   byte(eth::Instruction::ISZERO), // after this we have 10 + 8 >= 4
					   byte(eth::Instruction::DUP1),
					   byte(eth::Instruction::PUSH1), 0x20,
					   byte(eth::Instruction::JUMPI), // short-circuit if it is true
					   byte(eth::Instruction::POP),
					   byte(eth::Instruction::PUSH1), 0x2, byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::PUSH1), 0x9, byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::EQ),
					   byte(eth::Instruction::ISZERO), // after this we have 2 != 9
					   byte(eth::Instruction::JUMPDEST),
					   byte(eth::Instruction::PUSH1), 0x1,
					   byte(eth::Instruction::EQ),
					   byte(eth::Instruction::ISZERO)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(arithmetics)
{
	char const* sourceCode = "contract test {\n"
							 "  function f() { var x = ((((((((9 ^ 8) & 7) | 6) - 5) + 4) % 3) / 2) * 1); }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);
	bytes expectation({byte(eth::Instruction::PUSH1), 0x1,
					   byte(eth::Instruction::PUSH1), 0x2,
					   byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::PUSH1), 0x3,
					   byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::PUSH1), 0x4,
					   byte(eth::Instruction::PUSH1), 0x5,
					   byte(eth::Instruction::PUSH1), 0x6,
					   byte(eth::Instruction::PUSH1), 0x7,
					   byte(eth::Instruction::PUSH1), 0x8,
					   byte(eth::Instruction::PUSH1), 0x9,
					   byte(eth::Instruction::XOR),
					   byte(eth::Instruction::AND),
					   byte(eth::Instruction::OR),
					   byte(eth::Instruction::SUB),
					   byte(eth::Instruction::ADD),
					   byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::MOD),
					   byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::DIV),
					   byte(eth::Instruction::MUL)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(unary_operators)
{
	char const* sourceCode = "contract test {\n"
							 "  function f() { var x = !(~+- 1 == 2); }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({byte(eth::Instruction::PUSH1), 0x2, byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::PUSH1), 0x1,
					   byte(eth::Instruction::PUSH1), 0x0,
					   byte(eth::Instruction::SUB),
					   byte(eth::Instruction::NOT), byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::EQ),
					   byte(eth::Instruction::ISZERO)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(unary_inc_dec)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint a) { var x = --a ^ (a-- ^ (++a ^ a++)); }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode, {}, {{"test", "f", "a"}, {"test", "f", "x"}});

	// Stack: a, x
	bytes expectation({byte(eth::Instruction::DUP2),
					   byte(eth::Instruction::DUP1),
					   byte(eth::Instruction::PUSH1), 0x1,
					   byte(eth::Instruction::ADD),
					   // Stack here: a x a (a+1)
					   byte(eth::Instruction::SWAP3),
					   byte(eth::Instruction::POP), // first ++
					   // Stack here: (a+1) x a
					   byte(eth::Instruction::DUP3),
					   byte(eth::Instruction::PUSH1), 0x1,
					   byte(eth::Instruction::ADD),
					   // Stack here: (a+1) x a (a+2)
					   byte(eth::Instruction::SWAP3),
					   byte(eth::Instruction::POP),
					   // Stack here: (a+2) x a
					   byte(eth::Instruction::DUP3), // second ++
					   byte(eth::Instruction::XOR),
					   // Stack here: (a+2) x a^(a+2)
					   byte(eth::Instruction::DUP3),
					   byte(eth::Instruction::DUP1),
					   byte(eth::Instruction::PUSH1), 0x1,
					   byte(eth::Instruction::SWAP1),
					   byte(eth::Instruction::SUB),
					   // Stack here: (a+2) x a^(a+2) (a+2) (a+1)
					   byte(eth::Instruction::SWAP4),
					   byte(eth::Instruction::POP), // first --
					   byte(eth::Instruction::XOR),
					   // Stack here: (a+1) x a^(a+2)^(a+2)
					   byte(eth::Instruction::DUP3),
					   byte(eth::Instruction::PUSH1), 0x1,
					   byte(eth::Instruction::SWAP1),
					   byte(eth::Instruction::SUB),
					   // Stack here: (a+1) x a^(a+2)^(a+2) a
					   byte(eth::Instruction::SWAP3),
					   byte(eth::Instruction::POP), // second ++
					   // Stack here: a x a^(a+2)^(a+2)
					   byte(eth::Instruction::DUP3), // will change
					   byte(eth::Instruction::XOR)});
					   // Stack here: a x a^(a+2)^(a+2)^a
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(assignment)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint a, uint b) { (a += b) * 2; }"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode, {}, {{"test", "f", "a"}, {"test", "f", "b"}});

	// Stack: a, b
	bytes expectation({byte(eth::Instruction::PUSH1), 0x2, byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::DUP2),
					   byte(eth::Instruction::DUP4),
					   byte(eth::Instruction::ADD),
					   // Stack here: a b 2 a+b
					   byte(eth::Instruction::SWAP3),
					   byte(eth::Instruction::POP),
					   byte(eth::Instruction::DUP3),
					   // Stack here: a+b b 2 a+b
					   byte(eth::Instruction::MUL)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(function_call)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint a, uint b) { a += g(a + 1, b) * 2; }\n"
							 "  function g(uint a, uint b) returns (uint c) {}\n"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode, {{"test", "g"}},
										{{"test", "f", "a"}, {"test", "f", "b"}});

	// Stack: a, b
	bytes expectation({byte(eth::Instruction::PUSH1), 0x02, byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::PUSH1), 0x12,
					   byte(eth::Instruction::PUSH1), 0x01, byte(eth::Instruction::PUSH1), 0xff, byte(eth::Instruction::AND),
					   byte(eth::Instruction::DUP5),
					   byte(eth::Instruction::ADD),
					   // Stack here: a b 2 <ret label> (a+1)
					   byte(eth::Instruction::DUP4),
					   byte(eth::Instruction::PUSH1), 0x19,
					   byte(eth::Instruction::JUMP),
					   byte(eth::Instruction::JUMPDEST),
					   // Stack here: a b 2 g(a+1, b)
					   byte(eth::Instruction::MUL),
					   // Stack here: a b g(a+1, b)*2
					   byte(eth::Instruction::DUP3),
					   byte(eth::Instruction::ADD),
					   // Stack here: a b a+g(a+1, b)*2
					   byte(eth::Instruction::SWAP2),
					   byte(eth::Instruction::POP),
					   byte(eth::Instruction::DUP2),
					   byte(eth::Instruction::JUMPDEST)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(negative_literals_8bits)
{
	// these all fit in 8 bits
	char const* sourceCode = "contract test {\n"
							 "  function f() { int8 x = -0 + -1 + -0x01 + -127 + -128; }\n"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation(bytes({byte(eth::Instruction::PUSH32)}) + bytes(31, 0xff) + bytes(1, 0x80) +
					  bytes({byte(eth::Instruction::PUSH32)}) + bytes(31, 0xff) + bytes(1, 0x81) +
					  bytes({byte(eth::Instruction::PUSH32)}) + bytes(32, 0xff) +
					  bytes({byte(eth::Instruction::PUSH32)}) + bytes(32, 0xff) +
					  bytes({byte(eth::Instruction::PUSH1), 0x00,
							 byte(eth::Instruction::ADD),
							 byte(eth::Instruction::ADD),
							 byte(eth::Instruction::ADD),
							 byte(eth::Instruction::ADD)}));
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(negative_literals_16bits)
{
	// -1 should need 8 bits, -129 should need 16 bits, how many bits are used is visible
	// from the SIGNEXTEND opcodes
	char const* sourceCode = "contract test {\n"
							 "  function f() { int64 x = int64(-1 + -129); }\n"
							 "}\n";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation(bytes({byte(eth::Instruction::PUSH32)}) + bytes(31, 0xff) + bytes(1, 0x7f) +
					  bytes({byte(eth::Instruction::PUSH32)}) + bytes(32, 0xff) +
					  bytes({byte(eth::Instruction::PUSH1), 0x00,
							 byte(eth::Instruction::SIGNEXTEND),
							 byte(eth::Instruction::ADD),
							 byte(eth::Instruction::PUSH1), 0x01,
							 byte(eth::Instruction::SIGNEXTEND)}));
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
} // end namespaces

