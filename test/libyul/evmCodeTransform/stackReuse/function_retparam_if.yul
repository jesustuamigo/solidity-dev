{
    function f() -> x { pop(address()) if 1 { pop(callvalue()) } }
}
// ====
// stackOptimization: true
// EVMVersion: =current
// ----
//     /* "":0:70   */
//   stop
//     /* "":6:68   */
// tag_1:
//     /* "":22:23   */
//   0x00
//     /* "":6:68   */
//   swap1
//     /* "":44:45   */
//   0x01
//     /* "":30:39   */
//   address
//     /* "":26:40   */
//   pop
//     /* "":41:66   */
//   tag_2
//   jumpi
//     /* "":24:68   */
// tag_3:
//     /* "":6:68   */
//   jump	// out
//     /* "":46:66   */
// tag_2:
//     /* "":52:63   */
//   callvalue
//     /* "":48:64   */
//   pop
//     /* "":46:66   */
//   jump(tag_3)
