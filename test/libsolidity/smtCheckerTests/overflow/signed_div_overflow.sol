pragma experimental SMTChecker;

contract C  {
	function f(int x, int y) public pure returns (int) {
		return x / y;
	}
}
// ----
// Warning 1218: (110-115): CHC: Error trying to invoke SMT solver.
// Warning 3046: (110-115): BMC: Division by zero happens here.
// Warning 2661: (110-115): BMC: Overflow (resulting value larger than 0x80 * 2**248 - 1) happens here.
