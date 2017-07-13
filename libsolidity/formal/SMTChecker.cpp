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

#include <libsolidity/formal/SMTChecker.h>


#include <libsolidity/interface/ErrorReporter.h>

using namespace std;
using namespace dev;
using namespace dev::solidity;

SMTChecker::SMTChecker(ErrorReporter& _errorReporter, ReadFile::Callback const& _readFileCallback):
	m_interface(_readFileCallback),
	m_errorReporter(_errorReporter)
{
}

void SMTChecker::analyze(SourceUnit const& _source)
{
	bool pragmaFound = false;
	for (auto const& node: _source.nodes())
		if (auto const* pragma = dynamic_cast<PragmaDirective const*>(node.get()))
			if (pragma->literals()[0] == "checkAssertionsZ3")
				pragmaFound = true;
	if (pragmaFound)
	{
		m_interface.reset();
		m_currentSequenceCounter.clear();
		_source.accept(*this);
	}
}

void SMTChecker::endVisit(VariableDeclaration const& _varDecl)
{
	if (_varDecl.value())
	{
		m_errorReporter.warning(
			_varDecl.location(),
			"Assertion checker does not yet support this."
		);
	}
	else if (_varDecl.isLocalOrReturn())
		createVariable(_varDecl, true);
	else if (_varDecl.isCallableParameter())
		createVariable(_varDecl, false);
}

bool SMTChecker::visit(FunctionDefinition const& _function)
{
	if (!_function.modifiers().empty() || _function.isConstructor())
		m_errorReporter.warning(
			_function.location(),
			"Assertion checker does not yet support constructors and functions with modifiers."
		);
	// TODO actually we probably also have to reset all local variables and similar things.
	m_currentFunction = &_function;
	m_interface.push();
	return true;
}

void SMTChecker::endVisit(FunctionDefinition const&)
{
	// TOOD we could check for "reachability", i.e. satisfiability here.
	// We only handle local variables, so we clear everything.
	// If we add storage variables, those should be cleared differently.
	m_currentSequenceCounter.clear();
	m_interface.pop();
	m_currentFunction = nullptr;
}

void SMTChecker::endVisit(VariableDeclarationStatement const& _varDecl)
{
	if (_varDecl.declarations().size() != 1)
		m_errorReporter.warning(
			_varDecl.location(),
			"Assertion checker does not yet support such variable declarations."
		);
	else if (knownVariable(*_varDecl.declarations()[0]) && _varDecl.initialValue())
		// TODO more checks?
		// TODO add restrictions about type (might be assignment from smaller type)
		m_interface.addAssertion(newValue(*_varDecl.declarations()[0]) == expr(*_varDecl.initialValue()));
	else
		m_errorReporter.warning(
			_varDecl.location(),
			"Assertion checker does not yet implement such variable declarations."
		);
}

void SMTChecker::endVisit(ExpressionStatement const&)
{
}

void SMTChecker::endVisit(Assignment const& _assignment)
{
	if (_assignment.assignmentOperator() != Token::Value::Assign)
		m_errorReporter.warning(
			_assignment.location(),
			"Assertion checker does not yet implement compound assignment."
		);
	else if (_assignment.annotation().type->category() != Type::Category::Integer)
		m_errorReporter.warning(
			_assignment.location(),
			"Assertion checker does not yet implement type " + _assignment.annotation().type->toString()
		);
	else if (Identifier const* identifier = dynamic_cast<Identifier const*>(&_assignment.leftHandSide()))
	{
		Declaration const* decl = identifier->annotation().referencedDeclaration;
		if (knownVariable(*decl))
			// TODO more checks?
			// TODO add restrictions about type (might be assignment from smaller type)
			m_interface.addAssertion(newValue(*decl) == expr(_assignment.rightHandSide()));
		else
			m_errorReporter.warning(
				_assignment.location(),
				"Assertion checker does not yet implement such assignments."
			);
	}
	else
		m_errorReporter.warning(
			_assignment.location(),
			"Assertion checker does not yet implement such assignments."
		);
}

void SMTChecker::endVisit(TupleExpression const& _tuple)
{
	if (_tuple.isInlineArray() || _tuple.components().size() != 1)
		m_errorReporter.warning(
			_tuple.location(),
			"Assertion checker does not yet implement tules and inline arrays."
		);
	else
		m_interface.addAssertion(expr(_tuple) == expr(*_tuple.components()[0]));
}

void SMTChecker::endVisit(BinaryOperation const& _op)
{
	if (Token::isArithmeticOp(_op.getOperator()))
		arithmeticOperation(_op);
	else if (Token::isCompareOp(_op.getOperator()))
		compareOperation(_op);
	else if (Token::isBooleanOp(_op.getOperator()))
		booleanOperation(_op);
	else
		m_errorReporter.warning(
			_op.location(),
			"Assertion checker does not yet implement this operator."
		);
}

void SMTChecker::endVisit(FunctionCall const& _funCall)
{
	FunctionType const& funType = dynamic_cast<FunctionType const&>(*_funCall.expression().annotation().type);

	std::vector<ASTPointer<Expression const>> const args = _funCall.arguments();
	if (funType.kind() == FunctionType::Kind::Assert)
	{
		solAssert(args.size() == 1, "");
		solAssert(args[0]->annotation().type->category() == Type::Category::Bool, "");
		checkCondition(!(expr(*args[0])), _funCall.location(), "Assertion violation");
		m_interface.addAssertion(expr(*args[0]));
	}
	else if (funType.kind() == FunctionType::Kind::Require)
	{
		solAssert(args.size() == 1, "");
		solAssert(args[0]->annotation().type->category() == Type::Category::Bool, "");
		m_interface.addAssertion(expr(*args[0]));
		checkCondition(!(expr(*args[0])), _funCall.location(), "Unreachable code");
		// TODO is there something meaningful we can check here?
		// We can check whether the condition is always fulfilled or never fulfilled.
	}
}

void SMTChecker::endVisit(Identifier const& _identifier)
{
	Declaration const* decl = _identifier.annotation().referencedDeclaration;
	solAssert(decl, "");
	if (dynamic_cast<IntegerType const*>(_identifier.annotation().type.get()))
	{
		m_interface.addAssertion(expr(_identifier) == currentValue(*decl));
		return;
	}
	else if (FunctionType const* fun = dynamic_cast<FunctionType const*>(_identifier.annotation().type.get()))
	{
		if (fun->kind() == FunctionType::Kind::Assert || fun->kind() == FunctionType::Kind::Require)
			return;
		// TODO for others, clear our knowledge about storage and memory
	}
	m_errorReporter.warning(
		_identifier.location(),
		"Assertion checker does not yet support the type of this expression (" +
		_identifier.annotation().type->toString() +
		")."
	);
}

void SMTChecker::endVisit(Literal const& _literal)
{
	Type const& type = *_literal.annotation().type;
	if (type.category() == Type::Category::Integer || type.category() == Type::Category::RationalNumber)
	{
		if (RationalNumberType const* rational = dynamic_cast<RationalNumberType const*>(&type))
			solAssert(!rational->isFractional(), "");

		m_interface.addAssertion(expr(_literal) == smt::Expression(type.literalValue(&_literal)));
	}
	else
		m_errorReporter.warning(
			_literal.location(),
			"Assertion checker does not yet support the type of this expression (" +
			_literal.annotation().type->toString() +
			")."
		);
}

void SMTChecker::arithmeticOperation(BinaryOperation const& _op)
{
	switch (_op.getOperator())
	{
	case Token::Add:
	case Token::Sub:
	case Token::Mul:
	{
		solAssert(_op.annotation().commonType, "");
		solAssert(_op.annotation().commonType->category() == Type::Category::Integer, "");
		smt::Expression left(expr(_op.leftExpression()));
		smt::Expression right(expr(_op.rightExpression()));
		Token::Value op = _op.getOperator();
		smt::Expression value(
			op == Token::Add ? left + right :
			op == Token::Sub ? left - right :
			/*op == Token::Mul*/ left * right
		);

		// Overflow check
		auto const& intType = dynamic_cast<IntegerType const&>(*_op.annotation().commonType);
		checkCondition(
			value < minValue(intType),
			_op.location(),
			"Underflow (resulting value less than " + intType.minValue().str() + ")",
			"value",
			&value
		);
		checkCondition(
			value > maxValue(intType),
			_op.location(),
			"Overflow (resulting value larger than " + intType.maxValue().str() + ")",
			"value",
			&value
		);

		m_interface.addAssertion(expr(_op) == value);
		break;
	}
	default:
		m_errorReporter.warning(
			_op.location(),
			"Assertion checker does not yet implement this operator."
		);
	}
}

void SMTChecker::compareOperation(BinaryOperation const& _op)
{
	solAssert(_op.annotation().commonType, "");
	if (_op.annotation().commonType->category() == Type::Category::Integer)
	{
		smt::Expression left(expr(_op.leftExpression()));
		smt::Expression right(expr(_op.rightExpression()));
		Token::Value op = _op.getOperator();
		smt::Expression value = (
			op == Token::Equal ? (left == right) :
			op == Token::NotEqual ? (left != right) :
			op == Token::LessThan ? (left < right) :
			op == Token::LessThanOrEqual ? (left <= right) :
			op == Token::GreaterThan ? (left > right) :
			/*op == Token::GreaterThanOrEqual*/ (left >= right)
		);
		// TODO: check that other values for op are not possible.
		m_interface.addAssertion(expr(_op) == value);
	}
	else
		m_errorReporter.warning(
			_op.location(),
			"Assertion checker does not yet implement the type " + _op.annotation().commonType->toString() + " for comparisons"
		);
}

void SMTChecker::booleanOperation(BinaryOperation const& _op)
{
	solAssert(_op.getOperator() == Token::And || _op.getOperator() == Token::Or, "");
	solAssert(_op.annotation().commonType, "");
	if (_op.annotation().commonType->category() == Type::Category::Bool)
	{
		if (_op.getOperator() == Token::And)
			m_interface.addAssertion(expr(_op) == expr(_op.leftExpression()) && expr(_op.rightExpression()));
		else
			m_interface.addAssertion(expr(_op) == expr(_op.leftExpression()) || expr(_op.rightExpression()));
	}
	else
		m_errorReporter.warning(
			_op.location(),
			"Assertion checker does not yet implement the type " + _op.annotation().commonType->toString() + " for boolean operations"
		);
}

void SMTChecker::checkCondition(
	smt::Expression _condition,
	SourceLocation const& _location,
	string const& _description,
	string const& _additionalValueName,
	smt::Expression* _additionalValue
)
{
	m_interface.push();
	m_interface.addAssertion(_condition);

	vector<smt::Expression> expressionsToEvaluate;
	if (m_currentFunction)
	{
		if (_additionalValue)
			expressionsToEvaluate.emplace_back(*_additionalValue);
		for (auto const& param: m_currentFunction->parameters())
			if (knownVariable(*param))
				expressionsToEvaluate.emplace_back(currentValue(*param));
		for (auto const& var: m_currentFunction->localVariables())
			if (knownVariable(*var))
				expressionsToEvaluate.emplace_back(currentValue(*var));
	}
	smt::CheckResult result;
	vector<string> values;
	tie(result, values) = m_interface.check(expressionsToEvaluate);
	switch (result)
	{
	case smt::CheckResult::SAT:
	{
		std::ostringstream message;
		message << _description << " happens here";
		size_t i = 0;
		if (m_currentFunction)
		{
			message << " for:\n";
			if (_additionalValue)
				message << "  " << _additionalValueName << " = " << values.at(i++) << "\n";
			for (auto const& param: m_currentFunction->parameters())
				if (knownVariable(*param))
					message << "  " << param->name() << " = " << values.at(i++) << "\n";
			for (auto const& var: m_currentFunction->localVariables())
				if (knownVariable(*var))
					message << "  " << var->name() << " = " << values.at(i++) << "\n";
		}
		else
			message << ".";
		m_errorReporter.warning(_location, message.str());
		break;
	}
	case smt::CheckResult::UNSAT:
		break;
	case smt::CheckResult::UNKNOWN:
		m_errorReporter.warning(_location, _description + " might happen here.");
		break;
	case smt::CheckResult::ERROR:
		m_errorReporter.warning(_location, "Error trying to invoke SMT solver.");
		break;
	default:
		solAssert(false, "");
	}
	m_interface.pop();
}

void SMTChecker::createVariable(VariableDeclaration const& _varDecl, bool _setToZero)
{
	if (auto intType = dynamic_cast<IntegerType const*>(_varDecl.type().get()))
	{
		solAssert(m_currentSequenceCounter.count(&_varDecl) == 0, "");
		solAssert(m_z3Variables.count(&_varDecl) == 0, "");
		m_currentSequenceCounter[&_varDecl] = 0;
		m_z3Variables.emplace(&_varDecl, m_interface.newFunction(uniqueSymbol(_varDecl), smt::Sort::Int, smt::Sort::Int));
		if (_setToZero)
			m_interface.addAssertion(currentValue(_varDecl) == 0);
		else
		{
			m_interface.addAssertion(currentValue(_varDecl) >= minValue(*intType));
			m_interface.addAssertion(currentValue(_varDecl) <= maxValue(*intType));
		}
	}
	else
		m_errorReporter.warning(
			_varDecl.location(),
			"Assertion checker does not yet support the type of this variable."
		);
}

string SMTChecker::uniqueSymbol(Declaration const& _decl)
{
	return _decl.name() + "_" + to_string(_decl.id());
}

string SMTChecker::uniqueSymbol(Expression const& _expr)
{
	return "expr_" + to_string(_expr.id());
}

bool SMTChecker::knownVariable(Declaration const& _decl)
{
	return m_currentSequenceCounter.count(&_decl);
}

smt::Expression SMTChecker::currentValue(Declaration const& _decl)
{
	solAssert(m_currentSequenceCounter.count(&_decl), "");
	return var(_decl)(m_currentSequenceCounter.at(&_decl));
}

smt::Expression SMTChecker::newValue(const Declaration& _decl)
{
	solAssert(m_currentSequenceCounter.count(&_decl), "");
	m_currentSequenceCounter[&_decl]++;
	return currentValue(_decl);
}

smt::Expression SMTChecker::minValue(IntegerType const& _t)
{
	return smt::Expression(_t.minValue());
}

smt::Expression SMTChecker::maxValue(IntegerType const& _t)
{
	return smt::Expression(_t.maxValue());
}

smt::Expression SMTChecker::expr(Expression const& _e)
{
	if (!m_z3Expressions.count(&_e))
	{
		solAssert(_e.annotation().type, "");
		switch (_e.annotation().type->category())
		{
		case Type::Category::RationalNumber:
		{
			if (RationalNumberType const* rational = dynamic_cast<RationalNumberType const*>(_e.annotation().type.get()))
				solAssert(!rational->isFractional(), "");
			m_z3Expressions.emplace(&_e, m_interface.newInteger(uniqueSymbol(_e)));
			break;
		}
		case Type::Category::Integer:
			m_z3Expressions.emplace(&_e, m_interface.newInteger(uniqueSymbol(_e)));
			break;
		case Type::Category::Bool:
			m_z3Expressions.emplace(&_e, m_interface.newBool(uniqueSymbol(_e)));
			break;
		default:
			solAssert(false, "Type not implemented.");
		}
	}
	return m_z3Expressions.at(&_e);
}

smt::Expression SMTChecker::var(Declaration const& _decl)
{
	solAssert(m_z3Variables.count(&_decl), "");
	return m_z3Variables.at(&_decl);
}
