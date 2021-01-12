pragma experimental SMTChecker;

contract C
{
	uint[10] array;
	function f(uint x, uint y) public {
		array[x] = 200;
		require(x == y);
		assert(array[y] > 300);
	}
}
// ----
// Warning 6328: (139-161): CHC: Assertion violation happens here.\nCounterexample:\narray = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]\nx = 38\ny = 38\n\nTransaction trace:\nC.constructor()\nState: array = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]\nC.f(38, 38)
