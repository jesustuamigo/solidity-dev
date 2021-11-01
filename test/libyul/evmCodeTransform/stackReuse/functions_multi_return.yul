{
    function f(a, b) -> t { }
    function g() -> r, s { }
    let x := f(1, 2)
    x := f(3, 4)
    let y, z := g()
    y, z := g()
    let unused := 7
}
// ====
// stackOptimization: true
// ----
//     /* "":74:81   */
//   tag_3
//     /* "":79:80   */
//   0x02
//     /* "":76:77   */
//   0x01
//     /* "":74:81   */
//   tag_1
//   jump	// in
// tag_3:
//     /* "":91:98   */
//   pop
//   tag_4
//     /* "":96:97   */
//   0x04
//     /* "":93:94   */
//   0x03
//     /* "":91:98   */
//   tag_1
//   jump	// in
// tag_4:
//     /* "":115:118   */
//   pop
//   tag_5
//   tag_2
//   jump	// in
// tag_5:
//     /* "":131:134   */
//   pop
//   pop
//   tag_6
//   tag_2
//   jump	// in
// tag_6:
//     /* "":139:154   */
//   pop
//   pop
//     /* "":153:154   */
//   0x07
//     /* "":0:156   */
//   stop
//     /* "":6:31   */
// tag_1:
//   pop
//   pop
//     /* "":26:27   */
//   0x00
//     /* "":6:31   */
//   swap1
//   jump	// out
//     /* "":36:60   */
// tag_2:
//     /* "":55:56   */
//   0x00
//     /* "":52:53   */
//   0x00
//     /* "":36:60   */
//   swap2
//   jump	// out
