/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Analysis part of inline assembly.
 */

#pragma once

#include <libsolidity/inlineasm/AsmStack.h>

#include <libsolidity/interface/Exceptions.h>

#include <boost/variant.hpp>

#include <functional>
#include <memory>

namespace dev
{
namespace solidity
{
namespace assembly
{

struct Literal;
struct Block;
struct Label;
struct FunctionalInstruction;
struct FunctionalAssignment;
struct VariableDeclaration;
struct Instruction;
struct Identifier;
struct Assignment;
struct FunctionDefinition;
struct FunctionCall;

struct Scope;

/**
 * Performs the full analysis stage, calls the ScopeFiller internally, then resolves
 * references and performs other checks.
 * If all these checks pass, code generation should not throw errors.
 */
class AsmAnalyzer: public boost::static_visitor<bool>
{
public:
	using Scopes = std::map<assembly::Block const*, std::shared_ptr<Scope>>;
	AsmAnalyzer(Scopes& _scopes, ErrorList& _errors, ExternalIdentifierAccess::Resolver const& _resolver);

	bool analyze(assembly::Block const& _block);

	bool operator()(assembly::Instruction const&);
	bool operator()(assembly::Literal const& _literal);
	bool operator()(assembly::Identifier const&);
	bool operator()(assembly::FunctionalInstruction const& _functionalInstruction);
	bool operator()(assembly::Label const&) { return true; }
	bool operator()(assembly::Assignment const&);
	bool operator()(assembly::FunctionalAssignment const& _functionalAssignment);
	bool operator()(assembly::VariableDeclaration const& _variableDeclaration);
	bool operator()(assembly::FunctionDefinition const& _functionDefinition);
	bool operator()(assembly::FunctionCall const& _functionCall);
	bool operator()(assembly::Block const& _block);

private:
	/// Verifies that a variable to be assigned to exists and has the same size
	/// as the value, @a _valueSize, unless that is equal to -1.
	bool checkAssignment(assembly::Identifier const& _assignment, size_t _valueSize = size_t(-1));
	bool expectDeposit(int _deposit, int _oldHeight, SourceLocation const& _location);
	Scope& scope(assembly::Block const* _block);

	/// Number of excess stack slots generated by function arguments to take into account for
	/// next block.
	int m_virtualVariablesInNextBlock = 0;
	int m_stackHeight = 0;
	ExternalIdentifierAccess::Resolver const& m_resolver;
	Scope* m_currentScope = nullptr;
	Scopes& m_scopes;
	ErrorList& m_errors;
};

}
}
}
