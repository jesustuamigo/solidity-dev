pragma experimental SMTChecker;

contract C {

	struct S {
		uint x;
	}
	S s;

	function check() public {
		require(s.x == 0);
		conditional_increment();
		assert(s.x == 1); // should fail;
		assert(s.x == 0); // should hold;
	}

	function conditional_increment() internal {
		if (s.x == 0) {
			return;
		}
		s.x = 1;
	}
}
// ----
// Warning 6328: (156-172): CHC: Assertion violation happens here.\nCounterexample:\ns = {x: 0}\n\nTransaction trace:\nC.constructor()\nState: s = {x: 0}\nC.check()\n  C.conditional_increment() -- internal call
