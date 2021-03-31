contract C
{
	mapping (uint => uint) map;
	function f(uint x) public {
		map[x] = 2;
		uint a = ++map[x];
		assert(map[x] == 3);
		assert(a == 3);
		uint b = map[x]++;
		assert(map[x] == 4);
		// Should fail.
		assert(b < 3);
	}
}
// ====
// SMTEngine: all
// SMTIgnoreCex: yes
// ----
// Warning 6328: (211-224): CHC: Assertion violation happens here.
