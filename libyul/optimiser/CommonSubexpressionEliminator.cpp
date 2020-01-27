/*(
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
 * Optimisation stage that replaces expressions known to be the current value of a variable
 * in scope by a reference to that variable.
 */

#include <libyul/optimiser/CommonSubexpressionEliminator.h>

#include <libyul/optimiser/Metrics.h>
#include <libyul/optimiser/SyntacticalEquality.h>
#include <libyul/optimiser/CallGraphGenerator.h>
#include <libyul/optimiser/Semantics.h>
#include <libyul/SideEffects.h>
#include <libyul/Exceptions.h>
#include <libyul/AsmData.h>
#include <libyul/Dialect.h>

using namespace std;
using namespace solidity;
using namespace solidity::yul;
using namespace solidity::util;

void CommonSubexpressionEliminator::run(OptimiserStepContext& _context, Block& _ast)
{
	CommonSubexpressionEliminator cse{
		_context.dialect,
		SideEffectsPropagator::sideEffects(_context.dialect, CallGraphGenerator::callGraph(_ast))
	};
	cse(_ast);
}

CommonSubexpressionEliminator::CommonSubexpressionEliminator(
	Dialect const& _dialect,
	map<YulString, SideEffects> _functionSideEffects
):
	DataFlowAnalyzer(_dialect, std::move(_functionSideEffects))
{
}

void CommonSubexpressionEliminator::visit(Expression& _e)
{
	bool descend = true;
	// If this is a function call to a function that requires literal arguments,
	// do not try to simplify there.
	if (holds_alternative<FunctionCall>(_e))
		if (BuiltinFunction const* builtin = m_dialect.builtin(std::get<FunctionCall>(_e).functionName.name))
			if (builtin->literalArguments)
				// We should not modify function arguments that have to be literals
				// Note that replacing the function call entirely is fine,
				// if the function call is movable.
				descend = false;

	// We visit the inner expression first to first simplify inner expressions,
	// which hopefully allows more matches.
	// Note that the DataFlowAnalyzer itself only has code for visiting Statements,
	// so this basically invokes the AST walker directly and thus post-visiting
	// is also fine with regards to data flow analysis.
	if (descend)
		DataFlowAnalyzer::visit(_e);

	if (holds_alternative<Identifier>(_e))
	{
		Identifier& identifier = std::get<Identifier>(_e);
		YulString name = identifier.name;
		if (m_value.count(name))
		{
			assertThrow(m_value.at(name).value, OptimizerException, "");
			if (holds_alternative<Identifier>(*m_value.at(name).value))
			{
				YulString value = std::get<Identifier>(*m_value.at(name).value).name;
				assertThrow(inScope(value), OptimizerException, "");
				_e = Identifier{locationOf(_e), value};
			}
		}
	}
	else
	{
		// TODO this search is rather inefficient.
		for (auto const& [variable, value]: m_value)
		{
			assertThrow(value.value, OptimizerException, "");
			assertThrow(inScope(variable), OptimizerException, "");
			if (SyntacticallyEqual{}(_e, *value.value))
			{
				_e = Identifier{locationOf(_e), variable};
				break;
			}
		}
	}
}
