{
    function f() { }

    f()
}
// ====
// stackOptimization: true
// ----
//     /* "":28:31   */
//   tag_2
//   tag_1
//   jump	// in
// tag_2:
//     /* "":0:33   */
//   stop
//     /* "":6:22   */
// tag_1:
//   jump	// out
