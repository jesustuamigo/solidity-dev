pragma experimental SMTChecker;

contract C {
	function f() pure public {
		require(false);
		// This is not reachable.
		assert(false);
	}

	function g() pure public {
		require(false, "require message");
		// This is not reachable.
		assert(false);
	}

	function h(bool b) pure public {
		if (b)
			require(false);
		assert(!b);
	}

	// Check that arguments are evaluated.
	bool x = false;
	function m() view internal returns (string memory) {
		assert(x != true);
	}
	function i() public {
		x = true;
		require(false, m());
	}
}
// ----
// Warning 6328: (448-465): Assertion violation happens here.
