contract C {
	uint x;

	bool lock;
	modifier mutex {
		require(!lock);
		lock = true;
		_;
		lock = false;
	}

	function set(uint _x) mutex public {
		x = _x;
	}

	function f(address _a) public {
		uint y = x;
		_a.staticcall("aaaaa");
		assert(y == x); // should hold
	}
}
// ====
// SMTEngine: all
// ----
// Warning 9302: (212-234): Return value of low-level calls not used.
// Warning 2018: (164-271): Function state mutability can be restricted to view
// Info 1391: CHC: 1 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
