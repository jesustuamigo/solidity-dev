contract C {
    function f() public returns (uint256 a) {
        a = 0x4200;
        a >>= 8;
    }
}

// ====
// compileViaYul: also
// ----
// f() -> 0x42
