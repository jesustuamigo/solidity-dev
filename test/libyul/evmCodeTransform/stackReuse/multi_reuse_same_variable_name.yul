{ let z := mload(0) { let x := 1 x := 6 z := x } { let x := 2 z := x x := 4 } }
// ====
// stackOptimization: true
// EVMVersion: =current
// ----
//     /* "":17:18   */
//   0x00
//     /* "":11:19   */
//   mload
//     /* "":22:32   */
//   pop
//     /* "":31:32   */
//   0x01
//     /* "":33:39   */
//   pop
//     /* "":38:39   */
//   0x06
//     /* "":51:61   */
//   pop
//     /* "":60:61   */
//   0x02
//     /* "":69:75   */
//   pop
//     /* "":74:75   */
//   0x04
//     /* "":0:79   */
//   stop
