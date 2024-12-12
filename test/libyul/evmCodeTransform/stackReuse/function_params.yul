{
    function f(a, b) { }
}
// ====
// stackOptimization: true
// EVMVersion: =current
// ----
//     /* "":0:28   */
//   stop
//     /* "":6:26   */
// tag_1:
//   pop
//   pop
//   jump	// out
