contract C {
	struct S {
		uint x;
		uint[] a;
	}
	S s;
	function f(uint _x) public {
		s.x = _x;
		s.a.pop();
		s.a.push();
		s.a.push();
		s.a[0] = _x;
		assert(s.a[1] == s.a[0]);
		s.a.pop();
		s.a.pop();
	}
}
// ====
// SMTEngine: all
// SMTIgnoreCex: yes
// ----
// Warning 2529: (100-109): CHC: Empty array "pop" happens here.
// Warning 6328: (156-180): CHC: Assertion violation happens here.
// Info 1391: CHC: 5 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
