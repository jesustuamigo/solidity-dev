pragma experimental SMTChecker;

contract C {
	uint256[] x;
	constructor() public { x.push(42); }
	function f() public {
		x.push(23);
		assert(x[0] == 42 || x[0] == 23);
	}
}
