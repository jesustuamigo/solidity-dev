{
    extcodecopy(div(div(call(call(gas(),0x10000000000000000000000000000000000000000000000000, 0x100000000000000000000000000000000000000000000000000, 0x1000000000000000000000000000000000000000000000000000, 0x10000000000000000000000000000000000000000000000000000, 0x100000000000000000000000000000000000000000000000000000, 0x1000000000000000000000000000000000000000000000000000000), 0x10000000000000000000000000000000000000000000000000000000, 0x100000000000000000000000000000000000000000000000000000000, 0x1000000000000000000000000000000000000000000000000000000000, 0x10000000000000000000000000000000000000000000000000000000000, 0x100000000000000000000000000000000000000000000000000000000000, 0x1000000000000000000000000000000000000000000000000000000000000),0x10000000000000000000000000000000000000000000000000000000000000),0x100000000000000000000000000000000000000000000000000000000000000), 0x1000000000000000000000000000000000000000000000000000000000000000, 0x1000000000000000000000000000000000000000000000000000000000000001, 0x100000000000000000000000000000000000000000000000000000000000001)
}
// ====
// stackOptimization: true
// ----
//     /* "":1027:1092   */
//   0x0100000000000000000000000000000000000000000000000000000000000001
//     /* "":959:1025   */
//   0x1000000000000000000000000000000000000000000000000000000000000001
//     /* "":891:957   */
//   0x1000000000000000000000000000000000000000000000000000000000000000
//     /* "":823:888   */
//   0x0100000000000000000000000000000000000000000000000000000000000000
//     /* "":757:821   */
//   0x10000000000000000000000000000000000000000000000000000000000000
//     /* "":692:755   */
//   0x01000000000000000000000000000000000000000000000000000000000000
//     /* "":628:690   */
//   0x100000000000000000000000000000000000000000000000000000000000
//     /* "":565:626   */
//   0x010000000000000000000000000000000000000000000000000000000000
//     /* "":503:563   */
//   0x1000000000000000000000000000000000000000000000000000000000
//     /* "":442:501   */
//   0x0100000000000000000000000000000000000000000000000000000000
//     /* "":382:440   */
//   0x10000000000000000000000000000000000000000000000000000000
//     /* "":322:379   */
//   0x01000000000000000000000000000000000000000000000000000000
//     /* "":264:320   */
//   0x100000000000000000000000000000000000000000000000000000
//     /* "":207:262   */
//   0x010000000000000000000000000000000000000000000000000000
//     /* "":151:205   */
//   0x1000000000000000000000000000000000000000000000000000
//     /* "":96:149   */
//   0x0100000000000000000000000000000000000000000000000000
//     /* "":42:94   */
//   0x10000000000000000000000000000000000000000000000000
//     /* "":36:41   */
//   gas
//     /* "":31:380   */
//   call
//     /* "":26:756   */
//   call
//     /* "":22:822   */
//   div
//     /* "":18:889   */
//   div
//     /* "":6:1093   */
//   extcodecopy
//     /* "":0:1095   */
//   stop
