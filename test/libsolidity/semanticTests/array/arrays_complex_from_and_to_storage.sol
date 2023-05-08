contract Test {
    uint24[3][] public data;

    function set(uint24[3][] memory _data) public returns (uint256) {
        data = _data;
        return data.length;
    }

    function get() public returns (uint24[3][] memory) {
        return data;
    }
}
// ----
// set(uint24[3][]): 0x20, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12 -> 0x06
// gas irOptimized: 186537
// gas legacy: 211054
// gas legacyOptimized: 206042
// data(uint256,uint256): 0x02, 0x02 -> 0x09
// data(uint256,uint256): 0x05, 0x01 -> 0x11
// data(uint256,uint256): 0x06, 0x00 -> FAILURE
// get() -> 0x20, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12
