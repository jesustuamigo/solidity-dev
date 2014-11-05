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
 * Utilities for the solidity compiler.
 */

#include <utility>
#include <numeric>
#include <libsolidity/AST.h>
#include <libsolidity/Compiler.h>

using namespace std;

namespace dev {
namespace solidity {

void CompilerContext::initializeLocalVariables(unsigned _numVariables)
{
	if (_numVariables > 0)
	{
		*this << u256(0);
		for (unsigned i = 1; i < _numVariables; ++i)
			*this << eth::Instruction::DUP1;
		m_asm.adjustDeposit(-_numVariables);
	}
}

int CompilerContext::getStackPositionOfVariable(Declaration const& _declaration)
{
	auto res = find(begin(m_localVariables), end(m_localVariables), &_declaration);
	if (asserts(res != m_localVariables.end()))
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Variable not found on stack."));
	return end(m_localVariables) - res - 1 + m_asm.deposit();
}

eth::AssemblyItem CompilerContext::getFunctionEntryLabel(FunctionDefinition const& _function) const
{
	auto res = m_functionEntryLabels.find(&_function);
	if (asserts(res != m_functionEntryLabels.end()))
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Function entry label not found."));
	return res->second.tag();
}

}
}
