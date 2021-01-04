pragma experimental SMTChecker;

contract C  {
	function f(int x, int y) public pure returns (int) {
		return x + y;
	}
}
// ----
// Warning 3944: (110-115): CHC: Underflow (resulting value less than -57896044618658097711785492504343953926634992332820282019728792003956564819968) happens here.\nCounterexample:\n\nx = (- 1)\ny = (- 57896044618658097711785492504343953926634992332820282019728792003956564819968)\n = 0\n\nTransaction trace:\nC.constructor()\nC.f((- 1), (- 57896044618658097711785492504343953926634992332820282019728792003956564819968))
// Warning 4984: (110-115): CHC: Overflow (resulting value larger than 0x80 * 2**248 - 1) happens here.\nCounterexample:\n\nx = 1\ny = 57896044618658097711785492504343953926634992332820282019728792003956564819967\n = 0\n\nTransaction trace:\nC.constructor()\nC.f(1, 57896044618658097711785492504343953926634992332820282019728792003956564819967)
