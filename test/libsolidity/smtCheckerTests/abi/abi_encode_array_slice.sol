pragma experimental SMTChecker;
contract C {
	function abiEncodeSlice(bytes calldata data) external pure {
		bytes memory b1 = abi.encode(data);
		bytes memory b2 = abi.encode(data[0:]);
		// should hold but the engine cannot infer that data is fully equals data[0:] because each index is assigned separately
		assert(b1.length == b2.length); // fails for now

		// Disabled because of Spacer nondeterminism
		//bytes memory b3 = abi.encode(data[:data.length]);
		// should hold but the engine cannot infer that data is fully equals data[:data.length] because each index is assigned separately
		//assert(b1.length == b3.length); // fails for now

		bytes memory b4 = abi.encode(data[5:10]);
		assert(b1.length == b4.length); // should fail

		uint x = 5;
		uint y = 10;
		bytes memory b5 = abi.encode(data[x:y]);
		// should hold but the engine cannot infer that data[5:10] is fully equals data[x:y] because each index is assigned separately
		assert(b1.length == b5.length); // fails for now
	}
}
// ----
// Warning 6328: (311-341): CHC: Assertion violation happens here.
// Warning 1218: (694-724): CHC: Error trying to invoke SMT solver.
// Warning 6328: (694-724): CHC: Assertion violation might happen here.
// Warning 1218: (945-975): CHC: Error trying to invoke SMT solver.
// Warning 6328: (945-975): CHC: Assertion violation might happen here.
// Warning 4661: (694-724): BMC: Assertion violation happens here.
// Warning 4661: (945-975): BMC: Assertion violation happens here.
