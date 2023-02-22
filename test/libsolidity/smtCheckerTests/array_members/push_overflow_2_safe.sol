contract C {
	uint256[] x;
	function f(uint256 l) public {
		require(x.length == 0);
		x.push(42);
		x.push(84);
		for(uint256 i = 0; i < l; ++i)
			x.push(23);
		assert(x[0] == 42 || x[0] == 23);
	}
}
// ====
// SMTEngine: all
// ----
// Info 1391: CHC: 4 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
