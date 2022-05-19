pragma abicoder v2;

struct St0 {
    bytes el0;
}
contract C {
    function f() external returns (St0 memory) {
        St0 memory x;
        x.el0 = msg.data;
        return x;
    }

    function g() external returns (St0 memory) {
        bytes memory temp = msg.data;
        St0 memory x;
        x.el0 = temp;
        return x;
    }

    function hashes() external returns (bytes4, bytes4) {
        return (this.f.selector, this.g.selector);
    }

    function large(uint256, uint256, uint256, uint256) external returns (St0 memory) {
        St0 memory x;
        x.el0 = msg.data;
        return x;
    }

    function another_large(uint256, uint256, uint256, uint256) external returns (St0 memory) {
        bytes memory temp = msg.data;
        St0 memory x;
        x.el0 = temp;
        return x;
    }

}
// ----
// f() -> 0x20, 0x20, 4, 0x26121ff000000000000000000000000000000000000000000000000000000000
// g() -> 0x20, 0x20, 4, 0xe2179b8e00000000000000000000000000000000000000000000000000000000
// hashes() -> 0x26121ff000000000000000000000000000000000000000000000000000000000, 0xe2179b8e00000000000000000000000000000000000000000000000000000000
// large(uint256,uint256,uint256,uint256): 1, 2, 3, 4 -> 0x20, 0x20, 0x84, 0xe02492f800000000000000000000000000000000000000000000000000000000, 0x100000000000000000000000000000000000000000000000000000000, 0x200000000000000000000000000000000000000000000000000000000, 0x300000000000000000000000000000000000000000000000000000000, 0x400000000000000000000000000000000000000000000000000000000
// another_large(uint256,uint256,uint256,uint256): 1, 2, 3, 4 -> 0x20, 0x20, 0x84, 0x2a46f85a00000000000000000000000000000000000000000000000000000000, 0x100000000000000000000000000000000000000000000000000000000, 0x200000000000000000000000000000000000000000000000000000000, 0x300000000000000000000000000000000000000000000000000000000, 0x400000000000000000000000000000000000000000000000000000000
