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
// SPDX-License-Identifier: GPL-3.0

#include <libsolidity/codegen/ir/Common.h>
#include <libsolidity/ast/TypeProvider.h>

#include <libsolutil/CommonIO.h>

using namespace std;
using namespace solidity::util;
using namespace solidity::frontend;

YulArity YulArity::fromType(FunctionType const& _functionType)
{
	return YulArity{
		TupleType(_functionType.parameterTypesIncludingSelf()).sizeOnStack(),
		TupleType(_functionType.returnParameterTypes()).sizeOnStack()
	};
}

string IRNames::function(FunctionDefinition const& _function)
{
	return "fun_" + _function.name() + "_" + to_string(_function.id());
}

string IRNames::function(VariableDeclaration const& _varDecl)
{
	return "getter_fun_" + _varDecl.name() + "_" + to_string(_varDecl.id());
}

string IRNames::modifierInvocation(ModifierInvocation const& _modifierInvocation)
{
	// This uses the ID of the modifier invocation because it has to be unique
	// for each invocation.
	solAssert(!_modifierInvocation.name().path().empty(), "");
	string const& modifierName = _modifierInvocation.name().path().back();
	solAssert(!modifierName.empty(), "");
	return "modifier_" + modifierName + "_" + to_string(_modifierInvocation.id());
}

string IRNames::functionWithModifierInner(FunctionDefinition const& _function)
{
	return "fun_" + _function.name() + "_" + to_string(_function.id()) + "_inner";
}

string IRNames::creationObject(ContractDefinition const& _contract)
{
	return _contract.name() + "_" + toString(_contract.id());
}

string IRNames::runtimeObject(ContractDefinition const& _contract)
{
	return _contract.name() + "_" + toString(_contract.id()) + "_deployed";
}

string IRNames::internalDispatch(YulArity const& _arity)
{
	return "dispatch_internal"
		"_in_" + to_string(_arity.in) +
		"_out_" + to_string(_arity.out);
}

string IRNames::implicitConstructor(ContractDefinition const& _contract)
{
	return "constructor_" + _contract.name() + "_" + to_string(_contract.id());
}

string IRNames::constantValueFunction(VariableDeclaration const& _constant)
{
	solAssert(_constant.isConstant(), "");
	return "constant_" + _constant.name() + "_" + to_string(_constant.id());
}

string IRNames::localVariable(VariableDeclaration const& _declaration)
{
	return "vloc_" + _declaration.name() + '_' + std::to_string(_declaration.id());
}

string IRNames::localVariable(Expression const& _expression)
{
	return "expr_" + to_string(_expression.id());
}

string IRNames::trySuccessConditionVariable(Expression const& _expression)
{
	auto annotation = dynamic_cast<FunctionCallAnnotation const*>(&_expression.annotation());
	solAssert(annotation, "");
	solAssert(annotation->tryCall, "Parameter must be a FunctionCall with tryCall-annotation set.");

	return "trySuccessCondition_" + to_string(_expression.id());
}

string IRNames::tupleComponent(size_t _i)
{
	return "component_" + to_string(_i + 1);
}

string IRNames::zeroValue(Type const& _type, string const& _variableName)
{
	return "zero_value_for_type_" + _type.identifier() + _variableName;
}

FunctionDefinition const* IRHelpers::referencedFunctionDeclaration(Expression const& _expression)
{
	if (auto memberAccess = dynamic_cast<MemberAccess const*>(&_expression))
		return dynamic_cast<FunctionDefinition const*>(memberAccess->annotation().referencedDeclaration);
	else if (auto identifier = dynamic_cast<Identifier const*>(&_expression))
		return dynamic_cast<FunctionDefinition const*>(identifier->annotation().referencedDeclaration);
	else
		return nullptr;
}
