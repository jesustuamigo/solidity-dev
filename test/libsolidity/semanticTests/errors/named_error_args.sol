error E(uint a, uint b);
contract C {
    function f() public pure {
        revert E({b: 7, a: 2});
    }
}
// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// f() -> FAILURE, hex"85208890", hex"0000000000000000000000000000000000000000000000000000000000000002", hex"0000000000000000000000000000000000000000000000000000000000000007"
