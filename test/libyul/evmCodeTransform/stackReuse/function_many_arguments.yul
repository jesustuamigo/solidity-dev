{
    function f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) -> x {
        mstore(0x0100, a1)
        mstore(0x0120, a2)
        mstore(0x0140, a3)
        mstore(0x0160, a4)
        mstore(0x0180, a5)
        mstore(0x01A0, a6)
        mstore(0x01C0, a7)
        mstore(0x01E0, a8)
        mstore(0x0200, a9)
        mstore(0x0220, a10)
        mstore(0x0240, a11)
        mstore(0x0260, a12)
        mstore(0x0280, a13)
        mstore(0x02A0, a14)
        mstore(0x02C0, a15)
        mstore(0x02E0, a16)
        mstore(0x0300, a17)
        mstore(0x0320, a18)
        mstore(0x0340, a19)
        x := a20
    }
}
// ====
// stackOptimization: true
// EVMVersion: =current
// ----
//     /* "":0:662   */
//   stop
//     /* "":6:660   */
// tag_1:
//     /* "":130:136   */
//   0x0100
//     /* "":123:141   */
//   mstore
//     /* "":157:163   */
//   0x0120
//     /* "":150:168   */
//   mstore
//     /* "":184:190   */
//   0x0140
//     /* "":177:195   */
//   mstore
//     /* "":211:217   */
//   0x0160
//     /* "":204:222   */
//   mstore
//     /* "":238:244   */
//   0x0180
//     /* "":231:249   */
//   mstore
//     /* "":265:271   */
//   0x01a0
//     /* "":258:276   */
//   mstore
//     /* "":292:298   */
//   0x01c0
//     /* "":285:303   */
//   mstore
//     /* "":319:325   */
//   0x01e0
//     /* "":312:330   */
//   mstore
//     /* "":346:352   */
//   0x0200
//     /* "":339:357   */
//   mstore
//     /* "":373:379   */
//   0x0220
//     /* "":366:385   */
//   mstore
//     /* "":401:407   */
//   0x0240
//     /* "":394:413   */
//   mstore
//     /* "":429:435   */
//   0x0260
//     /* "":422:441   */
//   mstore
//     /* "":457:463   */
//   0x0280
//     /* "":450:469   */
//   mstore
//     /* "":485:491   */
//   0x02a0
//     /* "":478:497   */
//   mstore
//     /* "":513:519   */
//   0x02c0
//     /* "":506:525   */
//   mstore
//     /* "":541:547   */
//   0x02e0
//     /* "":534:553   */
//   mstore
//     /* "":569:575   */
//   0x0300
//     /* "":562:581   */
//   mstore
//     /* "":597:603   */
//   0x0320
//     /* "":590:609   */
//   mstore
//     /* "":625:631   */
//   0x0340
//     /* "":618:637   */
//   mstore
//     /* "":6:660   */
//   swap1
//   jump	// out
