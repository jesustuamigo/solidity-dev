{ let x := 1 x := 6 { let y := 2 y := 4 } }
// ====
// stackOptimization: true
// ----
//     /* "":11:12   */
//   0x01
//     /* "":13:19   */
//   pop
//     /* "":18:19   */
//   0x06
//     /* "":22:32   */
//   pop
//     /* "":31:32   */
//   0x02
//     /* "":33:39   */
//   pop
//     /* "":38:39   */
//   0x04
//     /* "":0:43   */
//   stop
