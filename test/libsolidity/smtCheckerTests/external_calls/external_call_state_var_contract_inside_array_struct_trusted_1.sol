contract D {
	uint public x;
}

contract C {
	struct S {
		D d;
	}
	S[] ss;
	constructor() {
		ss.push(S(new D()));
		assert(ss[0].d.x() == 0); // should hold
	}
	function f() public view {
		assert(ss[0].d.x() == 0); // should hold, but fails because we havoc the state
	}
}
// ====
// SMTEngine: chc
// SMTExtCalls: trusted
// SMTTargets: assert
// ----
// Warning 6328: (192-216): CHC: Assertion violation happens here.
// Info 1391: CHC: 1 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
