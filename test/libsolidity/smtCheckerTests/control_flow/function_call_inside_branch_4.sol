pragma experimental SMTChecker;

contract C
{
	function f() public pure {
		if (true) {
			address a = g();
			assert(a == address(0));
		}
		if (true) {
			address a = h();
			assert(a == address(0));
		}

	}
	function g() public pure returns (address) {
		address a;
		a = address(0);
		return a;
	}
	function h() public pure returns (address) {
		address a;
		return a;
	}

}
// ----
// Warning 5084: (275-285): Type conversion is not yet fully supported and might yield false positives.
// Warning 5084: (123-133): Type conversion is not yet fully supported and might yield false positives.
// Warning 5084: (189-199): Type conversion is not yet fully supported and might yield false positives.
// Warning 5084: (275-285): Type conversion is not yet fully supported and might yield false positives.
