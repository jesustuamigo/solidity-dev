pragma experimental SMTChecker;

contract C
{
	bool b;
	function f() public {
		if ((b = true) || (b == false)) {}
		if ((b == true) || (b = false)) {}
		if ((b = true) || (b = false)) {}
		if ((b == true) || (b == false)) {}
		if ((b = false) || b) {}
	}
}
// ----
// Warning 6838: (84-110): BMC: Condition is always true.
// Warning 6838: (121-147): BMC: Condition is always true.
// Warning 6838: (158-183): BMC: Condition is always true.
// Warning 6838: (194-221): BMC: Condition is always true.
// Warning 6838: (232-248): BMC: Condition is always false.
