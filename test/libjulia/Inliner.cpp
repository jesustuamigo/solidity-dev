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
 * @date 2017
 * Unit tests for the iulia function inliner.
 */

#include <test/libjulia/Common.h>

#include <libjulia/optimiser/InlinableFunctionFilter.h>
#include <libjulia/optimiser/FunctionalInliner.h>

#include <libsolidity/inlineasm/AsmPrinter.h>

#include <boost/test/unit_test.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace std;
using namespace dev;
using namespace dev::julia;
using namespace dev::julia::test;
using namespace dev::solidity::assembly;

namespace
{
string inlinableFunctions(string const& _source)
{
	auto ast = disambiguate(_source);

	InlinableFunctionFilter filter;
	filter(ast);

	return boost::algorithm::join(
		filter.inlinableFunctions() | boost::adaptors::map_keys,
		","
	);
}

string inlineFunctions(string const& _source, bool _julia = true)
{
	auto ast = disambiguate(_source, _julia);
	FunctionalInliner(ast).run();
	return AsmPrinter(_julia)(ast);
}
}

BOOST_AUTO_TEST_SUITE(IuliaInlinableFunctionFilter)

BOOST_AUTO_TEST_CASE(smoke_test)
{
	BOOST_CHECK_EQUAL(inlinableFunctions("{ }"), "");
}

BOOST_AUTO_TEST_CASE(simple)
{
	BOOST_CHECK_EQUAL(inlinableFunctions("{ function f() -> x:u256 { x := 2:u256 } }"), "f");
	BOOST_CHECK_EQUAL(inlinableFunctions(R"({
		function g(a:u256) -> b:u256 { b := a }
		function f() -> x:u256 { x := g(2:u256) }
	})"), "f,g");
}

BOOST_AUTO_TEST_CASE(simple_inside_structures)
{
	BOOST_CHECK_EQUAL(inlinableFunctions(R"({
		switch 2:u256
		case 2:u256 {
			function g(a:u256) -> b:u256 { b := a }
			function f() -> x:u256 { x := g(2:u256) }
		}
	})"), "f,g");
	BOOST_CHECK_EQUAL(inlinableFunctions(R"({
		for {
			function g(a:u256) -> b:u256 { b := a }
		} 1:u256 {
			function f() -> x:u256 { x := g(2:u256) }
		}
		{
			function h() -> y:u256 { y := 2:u256 }
		}
	})"), "f,g,h");
}

BOOST_AUTO_TEST_CASE(negative)
{
	BOOST_CHECK_EQUAL(inlinableFunctions("{ function f() -> x:u256 { } }"), "");
	BOOST_CHECK_EQUAL(inlinableFunctions("{ function f() -> x:u256 { x := 2:u256 {} } }"), "");
	BOOST_CHECK_EQUAL(inlinableFunctions("{ function f() -> x:u256 { x := f() } }"), "");
	BOOST_CHECK_EQUAL(inlinableFunctions("{ function f() -> x:u256 { x := x } }"), "");
	BOOST_CHECK_EQUAL(inlinableFunctions("{ function f() -> x:u256, y:u256 { x := 2:u256 } }"), "");
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(IuliaFunctionInliner)

BOOST_AUTO_TEST_CASE(simple)
{
	BOOST_CHECK_EQUAL(
		inlineFunctions("{ function f() -> x:u256 { x := 2:u256 } let y:u256 := f() }"),
		format("{ function f() -> x:u256 { x := 2:u256 } let y:u256 := 2:u256 }")
	);
}

BOOST_AUTO_TEST_CASE(with_args)
{
	BOOST_CHECK_EQUAL(
		inlineFunctions("{ function f(a:u256) -> x:u256 { x := a } let y:u256 := f(7:u256) }"),
		format("{ function f(a:u256) -> x:u256 { x := a } let y:u256 := 7:u256 }")
	);
}

BOOST_AUTO_TEST_CASE(no_inline_with_mload)
{
	// Does not inline because mload could be moved out of sequence
	BOOST_CHECK_EQUAL(
		inlineFunctions("{ function f(a) -> x { x := a } let y := f(mload(2)) }", false),
		format("{ function f(a) -> x { x := a } let y := f(mload(2)) }", false)
	);
}

BOOST_AUTO_TEST_CASE(complex_with_evm)
{
	BOOST_CHECK_EQUAL(
		inlineFunctions("{ function f(a) -> x { x := add(a, a) } let y := f(calldatasize()) }", false),
		format("{ function f(a) -> x { x := add(a, a) } let y := add(calldatasize(), calldatasize()) }", false)
	);
}

BOOST_AUTO_TEST_CASE(double_calls)
{
	BOOST_CHECK_EQUAL(
		inlineFunctions(R"({
			function f(a) -> x { x := add(a, a) }
			function g(b, c) -> y { y := mul(mload(c), f(b)) }
			let y := g(calldatasize(), 7)
		})", false),
		format(R"({
			function f(a) -> x { x := add(a, a) }
			function g(b, c) -> y { y := mul(mload(c), add(b, b)) }
			let y_1 := mul(mload(7), add(calldatasize(), calldatasize()))
		})", false)
	);
}

// TODO test double recursive calls

BOOST_AUTO_TEST_SUITE_END()
