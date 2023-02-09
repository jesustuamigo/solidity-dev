contract C {
	function f() public pure {
		assert(mulmod(2**256 - 1, 2, 14) == 2);
		uint y = 0;
		uint x = mulmod(2**256 - 1, 10, y);
		assert(x == 1);
	}
	function g(uint x, uint y, uint k) public pure returns (uint) {
		return mulmod(x, y, k);
	}
}
// ====
// SMTEngine: all
// ----
// Warning 4281: (108-133): CHC: Division by zero happens here.
// Warning 6328: (137-151): CHC: Assertion violation happens here.
// Warning 4281: (230-245): CHC: Division by zero happens here.
// Info 1391: CHC: 2 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
