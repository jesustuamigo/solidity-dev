{ let x := 1 mstore(3, 4) }
// ====
// stackOptimization: true
// EVMVersion: =current
// ----
//     /* "":11:12   */
//   0x01
//     /* "":13:25   */
//   pop
//     /* "":23:24   */
//   0x04
//     /* "":20:21   */
//   0x03
//     /* "":13:25   */
//   mstore
//     /* "":0:27   */
//   stop
