contract C {
    function f() public returns (address) {
        return address(f);
    }
}
// ----
// TypeError: (72-82): Explicit type conversion not allowed from "function () returns (address)" to "address".
