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
 * @author Lefteris Karapetsas <lefteris@ethdev.com>
 * @date 2015
 * Unit tests for Assembly Items from evmasm/Assembly.h
 */

#include <test/Common.h>

#include <liblangutil/SourceLocation.h>
#include <libevmasm/Assembly.h>

#include <liblangutil/Scanner.h>
#include <libsolidity/parsing/Parser.h>
#include <libsolidity/analysis/DeclarationTypeChecker.h>
#include <libsolidity/analysis/NameAndTypeResolver.h>
#include <libsolidity/codegen/Compiler.h>
#include <libsolidity/ast/AST.h>
#include <libsolidity/analysis/TypeChecker.h>
#include <liblangutil/ErrorReporter.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>

using namespace std;
using namespace solidity::langutil;
using namespace solidity::evmasm;

namespace solidity::frontend::test
{

namespace
{

evmasm::AssemblyItems compileContract(std::shared_ptr<CharStream> _sourceCode)
{
	ErrorList errors;
	ErrorReporter errorReporter(errors);
	Parser parser(errorReporter, solidity::test::CommonOptions::get().evmVersion());
	ASTPointer<SourceUnit> sourceUnit;
	BOOST_REQUIRE_NO_THROW(sourceUnit = parser.parse(make_shared<Scanner>(_sourceCode)));
	BOOST_CHECK(!!sourceUnit);

	GlobalContext globalContext;
	NameAndTypeResolver resolver(globalContext, solidity::test::CommonOptions::get().evmVersion(), errorReporter);
	DeclarationTypeChecker declarationTypeChecker(errorReporter, solidity::test::CommonOptions::get().evmVersion());
	solAssert(Error::containsOnlyWarnings(errorReporter.errors()), "");
	resolver.registerDeclarations(*sourceUnit);
	for (ASTPointer<ASTNode> const& node: sourceUnit->nodes())
		if (ContractDefinition* contract = dynamic_cast<ContractDefinition*>(node.get()))
		{
			BOOST_REQUIRE_NO_THROW(resolver.resolveNamesAndTypes(*contract));
			if (!Error::containsOnlyWarnings(errorReporter.errors()))
				return AssemblyItems();
		}
	for (ASTPointer<ASTNode> const& node: sourceUnit->nodes())
	{
		BOOST_REQUIRE_NO_THROW(declarationTypeChecker.check(*node));
		if (!Error::containsOnlyWarnings(errorReporter.errors()))
			return AssemblyItems();
	}
	for (ASTPointer<ASTNode> const& node: sourceUnit->nodes())
		if (ContractDefinition* contract = dynamic_cast<ContractDefinition*>(node.get()))
		{
			TypeChecker checker(solidity::test::CommonOptions::get().evmVersion(), errorReporter);
			BOOST_REQUIRE_NO_THROW(checker.checkTypeRequirements(*contract));
			if (!Error::containsOnlyWarnings(errorReporter.errors()))
				return AssemblyItems();
		}
	for (ASTPointer<ASTNode> const& node: sourceUnit->nodes())
		if (ContractDefinition* contract = dynamic_cast<ContractDefinition*>(node.get()))
		{
			Compiler compiler(
				solidity::test::CommonOptions::get().evmVersion(),
				RevertStrings::Default,
				solidity::test::CommonOptions::get().optimize ? OptimiserSettings::standard() : OptimiserSettings::minimal()
			);
			compiler.compileContract(*contract, map<ContractDefinition const*, shared_ptr<Compiler const>>{}, bytes());

			return compiler.runtimeAssemblyItems();
		}
	BOOST_FAIL("No contract found in source.");
	return AssemblyItems();
}

void printAssemblyLocations(AssemblyItems const& _items)
{
	auto printRepeated = [](SourceLocation const& _loc, size_t _repetitions)
	{
		cout <<
			"\t\tvector<SourceLocation>(" <<
			_repetitions <<
			", SourceLocation{" <<
			_loc.start <<
			", " <<
			_loc.end <<
			", make_shared<string>(\"" <<
			_loc.source->name() <<
			"\")}) +" << endl;
	};

	vector<SourceLocation> locations;
	for (auto const& item: _items)
		locations.push_back(item.location());
	size_t repetitions = 0;
	SourceLocation const* previousLoc = nullptr;
	for (size_t i = 0; i < locations.size(); ++i)
	{
		SourceLocation& loc = locations[i];
		if (previousLoc && *previousLoc == loc)
			repetitions++;
		else
		{
			if (previousLoc)
				printRepeated(*previousLoc, repetitions);
			previousLoc = &loc;
			repetitions = 1;
		}
	}
	if (previousLoc)
		printRepeated(*previousLoc, repetitions);
}

void checkAssemblyLocations(AssemblyItems const& _items, vector<SourceLocation> const& _locations)
{
	BOOST_CHECK_EQUAL(_items.size(), _locations.size());
	for (size_t i = 0; i < min(_items.size(), _locations.size()); ++i)
	{
		if (_items[i].location().start != _locations[i].start ||
			_items[i].location().end != _locations[i].end)
		{
			BOOST_CHECK_MESSAGE(false, "Location mismatch for item " + to_string(i) + ". Found the following locations:");
			printAssemblyLocations(_items);
			return;
		}
	}
}


} // end anonymous namespace

BOOST_AUTO_TEST_SUITE(Assembly)

BOOST_AUTO_TEST_CASE(location_test)
{
	auto sourceCode = make_shared<CharStream>(R"(
	contract test {
		function f() public returns (uint256 a) {
			return 16;
		}
	}
	)", "");
	AssemblyItems items = compileContract(sourceCode);
	bool hasShifts = solidity::test::CommonOptions::get().evmVersion().hasBitwiseShifting();

	auto codegenCharStream = make_shared<CharStream>("", "--CODEGEN--");

	vector<SourceLocation> locations;
	if (solidity::test::CommonOptions::get().optimize)
		locations =
			vector<SourceLocation>(31, SourceLocation{2, 82, sourceCode}) +
			vector<SourceLocation>(21, SourceLocation{20, 79, sourceCode}) +
			vector<SourceLocation>(1, SourceLocation{72, 74, sourceCode}) +
			vector<SourceLocation>(2, SourceLocation{20, 79, sourceCode});
	else
		locations =
			vector<SourceLocation>(hasShifts ? 31 : 32, SourceLocation{2, 82, sourceCode}) +
			vector<SourceLocation>(24, SourceLocation{20, 79, sourceCode}) +
			vector<SourceLocation>(1, SourceLocation{49, 58, sourceCode}) +
			vector<SourceLocation>(1, SourceLocation{72, 74, sourceCode}) +
			vector<SourceLocation>(2, SourceLocation{65, 74, sourceCode}) +
			vector<SourceLocation>(2, SourceLocation{20, 79, sourceCode});
	checkAssemblyLocations(items, locations);
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespaces
