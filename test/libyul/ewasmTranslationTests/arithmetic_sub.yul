{
  sstore(0, sub(0, 1))
  sstore(1, sub(1, not(0)))
  sstore(2, sub(0, 0))
  sstore(3, sub(1, 2))
  sstore(4, sub(0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff, 1))
  sstore(5, sub(
    0x8000000000000000000000000000000000000000000000000000000000000000, 1
  ))
  sstore(6, sub(not(0), 1))
  sstore(7, sub(not(0), not(0)))
  sstore(8, sub(0x1000000000000000000000000000000000000000000000000, 1))
  sstore(9, sub(0x100000000000000000000000000000000, 1))
  sstore(10, sub(0x10000000000000000, 1))
  sstore(11, sub(0x1000000000000000000000000000000000000000000000000, 3))
  sstore(12, sub(0x100000000000000000000000000000000, 3))
  sstore(13, sub(0x10000000000000000, 3))
}
// ----
// Trace:
// Memory dump:
//      0: 000000000000000000000000000000000000000000000000000000000000000d
//     20: 000000000000000000000000000000000000000000000000fffffffffffffffd
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000000: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
//   0000000000000000000000000000000000000000000000000000000000000001: 0000000000000000000000000000000000000000000000000000000000000002
//   0000000000000000000000000000000000000000000000000000000000000003: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
//   0000000000000000000000000000000000000000000000000000000000000004: fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe
//   0000000000000000000000000000000000000000000000000000000000000005: 7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
//   0000000000000000000000000000000000000000000000000000000000000006: fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe
//   0000000000000000000000000000000000000000000000000000000000000008: 0000000000000000ffffffffffffffffffffffffffffffffffffffffffffffff
//   0000000000000000000000000000000000000000000000000000000000000009: 00000000000000000000000000000000ffffffffffffffffffffffffffffffff
//   000000000000000000000000000000000000000000000000000000000000000a: 000000000000000000000000000000000000000000000000ffffffffffffffff
//   000000000000000000000000000000000000000000000000000000000000000b: 0000000000000000fffffffffffffffffffffffffffffffffffffffffffffffd
//   000000000000000000000000000000000000000000000000000000000000000c: 00000000000000000000000000000000fffffffffffffffffffffffffffffffd
//   000000000000000000000000000000000000000000000000000000000000000d: 000000000000000000000000000000000000000000000000fffffffffffffffd
