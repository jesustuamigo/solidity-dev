// This does not reuse the parameters for the return parameters
// We do not expect parameters to be fully unused, so the stack
// layout for a function is still fixed, even though parameters
// can be reused.
{
    function f(a, b, c, d) -> x, y { }
}
// ====
// stackOptimization: true
// ----
//     /* "":210:252   */
//   stop
//     /* "":216:250   */
// tag_1:
//   pop
//   pop
//   pop
//   pop
//     /* "":245:246   */
//   0x00
//     /* "":242:243   */
//   0x00
//     /* "":216:250   */
//   swap2
//   jump	// out
