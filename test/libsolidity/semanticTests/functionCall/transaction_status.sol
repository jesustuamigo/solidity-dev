contract test {
	function f() public { }
	function g() public { revert(); }
	function h() public { assert(false); }
}
// ====
// compileViaYul: also
// ----
// f() ->
// g() -> FAILURE
// h() -> FAILURE, hex"4e487b71", 0x01
