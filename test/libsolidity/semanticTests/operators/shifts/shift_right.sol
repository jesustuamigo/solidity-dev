contract C {
    function f(uint256 a, uint256 b) public returns (uint256) {
        return a >> b;
    }
}

// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// f(uint256,uint256): 0x4266, 0x0 -> 0x4266
// f(uint256,uint256): 0x4266, 0x8 -> 0x42
// f(uint256,uint256): 0x4266, 0x10 -> 0
// f(uint256,uint256): 0x4266, 0x11 -> 0
// f(uint256,uint256): 57896044618658097711785492504343953926634992332820282019728792003956564819968, 5 -> 1809251394333065553493296640760748560207343510400633813116524750123642650624
