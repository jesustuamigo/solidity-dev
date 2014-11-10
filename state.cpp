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
/** @file state.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2014
 * State test functions.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include "JsonSpiritHeaders.h"
#include <libdevcore/CommonIO.h>
#include <libethereum/BlockChain.h>
#include <libethereum/State.h>
#include <libethereum/ExtVM.h>
#include <libethereum/Defaults.h>
#include <libevm/VM.h>
#include "TestHelper.h"

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;
using namespace dev::eth;

namespace dev {  namespace test {



void doStateTests(json_spirit::mValue& v, bool _fillin)
{
	for (auto& i: v.get_obj())
	{
		cnote << i.first;
		mObject& o = i.second.get_obj();

		BOOST_REQUIRE(o.count("env") > 0);
		BOOST_REQUIRE(o.count("pre") > 0);
		BOOST_REQUIRE(o.count("transaction") > 0);

		ImportTest importer(o, _fillin);

		State theState = importer.m_statePre;
		bytes tx = importer.m_transaction.rlp();
		bytes output;

		try
		{
			theState.execute(tx, &output);
		}
		catch (Exception const& _e)
		{
			cnote << "state execution did throw an exception: " << diagnostic_information(_e);
		}
		catch (std::exception const& _e)
		{
			cnote << "state execution did throw an exception: " << _e.what();
		}

		if (_fillin)
			importer.exportTest(output, theState);
		else
		{
			BOOST_REQUIRE(o.count("post") > 0);
			BOOST_REQUIRE(o.count("out") > 0);

			// check output
			checkOutput(output, o);

			// check addresses
			auto expectedAddrs = importer.m_statePost.addresses();
			auto resultAddrs = theState.addresses();
			for (auto& expectedPair : expectedAddrs)
			{
				auto& expectedAddr = expectedPair.first;
				auto resultAddrIt = resultAddrs.find(expectedAddr);
				if (resultAddrIt == resultAddrs.end())
					BOOST_ERROR("Missing expected address " << expectedAddr);
				else
				{
					BOOST_CHECK_MESSAGE(importer.m_statePost.balance(expectedAddr) ==  theState.balance(expectedAddr), expectedAddr << ": incorrect balance " << theState.balance(expectedAddr) << ", expected " << importer.m_statePost.balance(expectedAddr));
					BOOST_CHECK_MESSAGE(importer.m_statePost.transactionsFrom(expectedAddr) ==  theState.transactionsFrom(expectedAddr), expectedAddr << ": incorrect txCount " << theState.transactionsFrom(expectedAddr) << ", expected " << importer.m_statePost.transactionsFrom(expectedAddr));
					BOOST_CHECK_MESSAGE(importer.m_statePost.code(expectedAddr) == theState.code(expectedAddr), expectedAddr << ": incorrect code");

					checkStorage(importer.m_statePost.storage(expectedAddr), theState.storage(expectedAddr), expectedAddr);
				}
			}
			checkAddresses<map<Address, u256> >(expectedAddrs, resultAddrs);
		}
	}
}
} }// Namespace Close

BOOST_AUTO_TEST_SUITE(StateTests)

BOOST_AUTO_TEST_CASE(stExample)
{
	dev::test::executeTests("stExample", "/StateTests", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stSystemOperationsTest)
{
	dev::test::executeTests("stSystemOperationsTest", "/StateTests", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stPreCompiledContracts)
{
	dev::test::executeTests("stPreCompiledContracts", "/StateTests", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(userDefinedFileState)
{
	for (int i = 1; i < boost::unit_test::framework::master_test_suite().argc; ++i)
	{
		string arg =  boost::unit_test::framework::master_test_suite().argv[i];
		if (arg == "--statetest")
		{
			if (i + 1 >= boost::unit_test::framework::master_test_suite().argc)
			{
				cnote << "Missing filename\nUsage: testeth --statetest <filename>\n";
				return;
			}
			string filename = boost::unit_test::framework::master_test_suite().argv[i+1];
			int currentVerbosity = g_logVerbosity;
			g_logVerbosity = 12;
			try
			{
				cnote << "Testing VM..." << "user defined test";
				json_spirit::mValue v;
				string s = asString(contents(filename));
				BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + filename + " is empty. ");
				json_spirit::read_string(s, v);
				dev::test::doStateTests(v, false);
			}
			catch (Exception const& _e)
			{
				BOOST_ERROR("Failed state test with Exception: " << diagnostic_information(_e));
			}
			catch (std::exception const& _e)
			{
				BOOST_ERROR("Failed state test with Exception: " << _e.what());
			}
			g_logVerbosity = currentVerbosity;
		}
		else
			continue;
	}
}

BOOST_AUTO_TEST_SUITE_END()
