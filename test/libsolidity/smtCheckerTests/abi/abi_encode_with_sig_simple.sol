pragma experimental SMTChecker;
contract C {
	function abiEncodeSimple(string memory sig, bool t, uint x, uint y, uint z, uint[] memory a, uint[] memory b) public pure {
		require(x == y);
		bytes memory b1 = abi.encodeWithSignature(sig, x, z, a);
		bytes memory b2 = abi.encodeWithSignature(sig, y, z, a);
		assert(b1.length == b2.length);

		// Disabled because of nondeterminism in Spacer Z3 4.8.9
		//bytes memory b3 = abi.encodeWithSignature(sig, y, z, b);
		//assert(b1.length == b3.length); // should fail

		bytes memory b4 = abi.encodeWithSignature(sig, t, z, a);
		assert(b1.length == b4.length); // should fail

		bytes memory b5 = abi.encodeWithSignature(sig, y, y, y, y, a, a, a);
		assert(b1.length != b5.length); // should fail
		assert(b1.length == b5.length); // should fail

		bytes memory b6 = abi.encodeWithSignature("f()", x, z, a);
		assert(b1.length == b6.length); // should fail
	}
}
// ----
// Warning 5667: (139-154): Unused function parameter. Remove or comment out the variable name to silence this warning.
// Warning 6328: (575-605): CHC: Assertion violation happens here.
// Warning 6328: (696-726): CHC: Assertion violation happens here.
// Warning 6328: (745-775): CHC: Assertion violation happens here.
// Warning 6328: (856-886): CHC: Assertion violation happens here.
