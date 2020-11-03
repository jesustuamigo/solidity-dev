pragma experimental ABIEncoderV2;

contract C {
    function f(uint a, bytes memory b, uint c)
            public pure returns (uint, uint, byte, uint) {
        return (a, b.length, b[3], c);
    }

    function f_external(uint a, bytes calldata b, uint c)
            external pure returns (uint, uint, byte, uint) {
        return (a, b.length, b[3], c);
    }
}
// ====
// compileViaYul: also
// ----
// f(uint256,bytes,uint256): 6, 0x60, 9, 7, "abcdefg" -> 6, 7, "d", 9
// f_external(uint256,bytes,uint256): 6, 0x60, 9, 7, "abcdefg" -> 6, 7, "d", 9