{
    mstore(0, 7)
    sstore(0, mload(0))
    mstore(sub(0, 1), sub(0, 1))
    sstore(1, mload(sub(0, 1)))
}
// ----
// Trace:
// Memory dump:
//      0: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff07
//   FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE0: 00000000000000000000000000000000000000000000000000000000000000ff
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000000: 0000000000000000000000000000000000000000000000000000000000000007
//   0000000000000000000000000000000000000000000000000000000000000001: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
// Transient storage dump:
