{
  mstore(0x20, 0x1234556677889900aa)
  return(0x20, 30)
  invalid()
}
// ----
// Trace:
//   RETURN() [000000000000000000000000000000000000000000000012345566778899]
// Memory dump:
//     60: 00000000000000000000000000000000000000000000001234556677889900aa
// Storage dump:
