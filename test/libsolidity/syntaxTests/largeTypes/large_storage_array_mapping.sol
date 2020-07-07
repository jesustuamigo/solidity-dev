contract C {
    mapping(uint => uint[2**100]) x;
}
// ----
// Warning 7325: (17-48): Type "uint256[1267650600228229401496703205376]" has large size and thus makes collisions likely. Either use mappings or dynamic arrays and allow their size to be increased only in small quantities per transaction.
