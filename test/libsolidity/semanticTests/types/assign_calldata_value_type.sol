contract C {
    function f(uint256 x) public pure returns (uint256, uint256) {
        uint256 b = x;
        x = 42;
        return (x, b);
    }
}
// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// f(uint256): 23 -> 42, 23
