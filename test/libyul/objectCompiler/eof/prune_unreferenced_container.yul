object "a" {
    code {
        mstore(0, eofcreate("c", 0, 0, 0, 0))
        return(0, 32)
    }

    object "b" {
        code {
            mstore(0, 0x1122334455667788990011223344556677889900112233445566778899001122)
            mstore(32, 0x1122334455667788990011223344556677889900112233445566778899001122)
        }
    }

    object "c" {
        code {
            mstore(0, 0x1122334455667788990011223344556677889900112233445566778899001122)
            mstore(32, 0x1122334455667788990011223344556677889900112233445566778899001122)
        }
    }
}

// ====
// EVMVersion: >=prague
// bytecodeFormat: >=EOFv1
// ----
// Assembly:
//     /* "source":80:81   */
//   0x00
//     /* "source":56:82   */
//   dup1
//   dup1
//   dup1
//   eofcreate{1}
//     /* "source":53:54   */
//   0x00
//     /* "source":46:83   */
//   mstore
//     /* "source":106:108   */
//   0x20
//     /* "source":103:104   */
//   0x00
//     /* "source":96:109   */
//   return
// stop
//
// sub_0: assembly {
//         /* "source":198:264   */
//       0x1122334455667788990011223344556677889900112233445566778899001122
//         /* "source":195:196   */
//       0x00
//         /* "source":188:265   */
//       mstore
//         /* "source":293:359   */
//       0x1122334455667788990011223344556677889900112233445566778899001122
//         /* "source":289:291   */
//       0x20
//         /* "source":282:360   */
//       mstore
//         /* "source":156:384   */
//       stop
// }
//
// sub_1: assembly {
//         /* "source":463:529   */
//       0x1122334455667788990011223344556677889900112233445566778899001122
//         /* "source":460:461   */
//       0x00
//         /* "source":453:530   */
//       mstore
//         /* "source":558:624   */
//       0x1122334455667788990011223344556677889900112233445566778899001122
//         /* "source":554:556   */
//       0x20
//         /* "source":547:625   */
//       mstore
//         /* "source":421:649   */
//       stop
// }
// Bytecode: ef0001010004020001000c030001005b040000000080ffff5f808080ec005f5260205ff3ef00010100040200010048040000000080ffff7f11223344556677889900112233445566778899001122334455667788990011225f527f112233445566778899001122334455667788990011223344556677889900112260205200
// Opcodes: 0xEF STOP ADD ADD STOP DIV MUL STOP ADD STOP 0xC SUB STOP ADD STOP JUMPDEST DIV STOP STOP STOP STOP DUP1 SELFDESTRUCT SELFDESTRUCT PUSH0 DUP1 DUP1 DUP1 EOFCREATE 0x0 PUSH0 MSTORE PUSH1 0x20 PUSH0 RETURN 0xEF STOP ADD ADD STOP DIV MUL STOP ADD STOP BASEFEE DIV STOP STOP STOP STOP DUP1 SELFDESTRUCT SELFDESTRUCT PUSH32 0x1122334455667788990011223344556677889900112233445566778899001122 PUSH0 MSTORE PUSH32 0x1122334455667788990011223344556677889900112233445566778899001122 PUSH1 0x20 MSTORE STOP
// SourceMappings: 80:1:0:-:0;56:26;;;;53:1;46:37;106:2;103:1;96:13
