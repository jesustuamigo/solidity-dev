contract C {
    function f() public pure returns (string[][] memory) {}
}
// ----
// TypeError 4957: (51-68): This type is only supported in ABI coder v2. Use "pragma abicoder v2;" to enable the feature.
