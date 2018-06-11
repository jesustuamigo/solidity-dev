contract C {
    mapping(uint => uint) x;
    function f() public returns (bool ret) {
        mapping(uint => uint) y = x;
        return x == y;
    }
}
// ----
// TypeError: (139-145): Operator == not compatible with types mapping(uint256 => uint256) and mapping(uint256 => uint256)
