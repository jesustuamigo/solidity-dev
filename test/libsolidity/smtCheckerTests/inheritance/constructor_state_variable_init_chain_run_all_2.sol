pragma experimental SMTChecker;
contract C {
	uint a;
	constructor(uint x) {
		a = x;
	}
}

abstract contract B is C {
	uint b;
	constructor(uint x) {
		b = x + 10;
	}
}

contract A is B {
	constructor(uint x) B(x) C(x + 2) {
		assert(a == x + 2);
		assert(b == x + 10);
		assert(b == x + 5);
	}
}

// ----
// Warning 4984: (157-163): Overflow (resulting value larger than 2**256 - 1) happens here
// Warning 4984: (217-222): Overflow (resulting value larger than 2**256 - 1) happens here
// Warning 4984: (240-245): Overflow (resulting value larger than 2**256 - 1) happens here
// Warning 4984: (262-268): Overflow (resulting value larger than 2**256 - 1) happens here
// Warning 4984: (285-290): Overflow (resulting value larger than 2**256 - 1) happens here
// Warning 6328: (273-291): Assertion violation happens here
