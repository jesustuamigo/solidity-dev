object "A" {
  code {
    let a_o := dataoffset("A")
    let a_s := datasize("A")
    let b_o := dataoffset("B")
    let b_s := datasize("B")
    let bc_o := dataoffset("B.C")
    let bc_s := datasize("B.C")
    let be_o := dataoffset("B.E")
    let be_s := datasize("B.E")
    let bcd_o := dataoffset("B.C.D")
    let bcd_s := datasize("B.C.D")

    sstore(0, a_o)
    sstore(32, a_s)
    sstore(64, b_o)
    sstore(96, b_s)
    sstore(128, bc_o)
    sstore(160, bc_s)
    sstore(192, be_o)
    sstore(224, be_s)
    sstore(256, bcd_o)
    sstore(288, bcd_s)
    return(0, 320)
  }

  data "data1" "Hello, World!"

  object "B" {
    code {
      let c_o := dataoffset("C")
      let c_s := datasize("C")
      let e_o := dataoffset("E")
      let e_s := datasize("E")
      let cd_o := dataoffset("C.D")
      let cd_s := datasize("C.D")

      sstore(0, c_o)
      sstore(32, c_s)
      sstore(64, e_o)
      sstore(96, e_s)
      sstore(128, cd_o)
      sstore(160, cd_s)
      return(0, 192)
    }
    object "C" {
      code {
        let d_o := dataoffset("D")
        let d_s := datasize("D")

        sstore(0, d_o)
        sstore(32, d_s)
        return(0, 64)
      }
      object "D" {
        code {
          invalid()
        }
      }
    }
    object "E" {
      code {
        invalid()
      }
    }
  }
}
// ----
// Assembly:
//   0x00
//     /* "source":26:52   */
//   bytecodeSize
//     /* "source":57:81   */
//   dataOffset(sub_0)
//     /* "source":86:112   */
//   dataSize(sub_0)
//     /* "source":117:141   */
//   dataOffset(sub_0.sub_0)
//     /* "source":146:175   */
//   dataSize(sub_0.sub_0)
//     /* "source":180:207   */
//   dataOffset(sub_0.sub_1)
//     /* "source":212:241   */
//   dataSize(sub_0.sub_1)
//     /* "source":246:273   */
//   dataOffset(sub_0.sub_0.sub_0)
//     /* "source":278:310   */
//   dataSize(sub_0.sub_0.sub_0)
//     /* "source":361:364   */
//   dup10
//     /* "source":358:359   */
//   0x00
//     /* "source":351:365   */
//   sstore
//     /* "source":381:384   */
//   dup9
//     /* "source":377:379   */
//   0x20
//     /* "source":370:385   */
//   sstore
//     /* "source":401:404   */
//   dup8
//     /* "source":397:399   */
//   0x40
//     /* "source":390:405   */
//   sstore
//     /* "source":421:424   */
//   dup7
//     /* "source":417:419   */
//   0x60
//     /* "source":410:425   */
//   sstore
//     /* "source":442:446   */
//   dup6
//     /* "source":437:440   */
//   0x80
//     /* "source":430:447   */
//   sstore
//     /* "source":464:468   */
//   dup5
//     /* "source":459:462   */
//   0xa0
//     /* "source":452:469   */
//   sstore
//     /* "source":486:490   */
//   dup4
//     /* "source":481:484   */
//   0xc0
//     /* "source":474:491   */
//   sstore
//     /* "source":508:512   */
//   dup3
//     /* "source":503:506   */
//   0xe0
//     /* "source":496:513   */
//   sstore
//     /* "source":530:535   */
//   dup2
//     /* "source":525:528   */
//   0x0100
//     /* "source":518:536   */
//   sstore
//     /* "source":553:558   */
//   dup1
//     /* "source":548:551   */
//   0x0120
//     /* "source":541:559   */
//   sstore
//     /* "source":574:577   */
//   0x0140
//     /* "source":571:572   */
//   0x00
//     /* "source":564:578   */
//   return
//     /* "source":20:582   */
//   pop
//   pop
//   pop
//   pop
//   pop
//   pop
//   pop
//   pop
//   pop
//   pop
// stop
// data_acaf3289d7b601cbd114fb36c4d29c85bbfd5e133f14cb355c3fd8d99367964f 48656c6c6f2c20576f726c6421
//
// sub_0: assembly {
//       dataOffset(sub_0)
//         /* "source":648:674   */
//       dataSize(sub_0)
//         /* "source":681:705   */
//       dataOffset(sub_1)
//         /* "source":712:738   */
//       dataSize(sub_1)
//         /* "source":745:769   */
//       dataOffset(sub_0.sub_0)
//         /* "source":776:805   */
//       dataSize(sub_0.sub_0)
//         /* "source":857:860   */
//       dup6
//         /* "source":854:855   */
//       0x00
//         /* "source":847:861   */
//       sstore
//         /* "source":879:882   */
//       dup5
//         /* "source":875:877   */
//       0x20
//         /* "source":868:883   */
//       sstore
//         /* "source":901:904   */
//       dup4
//         /* "source":897:899   */
//       0x40
//         /* "source":890:905   */
//       sstore
//         /* "source":923:926   */
//       dup3
//         /* "source":919:921   */
//       0x60
//         /* "source":912:927   */
//       sstore
//         /* "source":946:950   */
//       dup2
//         /* "source":941:944   */
//       0x80
//         /* "source":934:951   */
//       sstore
//         /* "source":970:974   */
//       dup1
//         /* "source":965:968   */
//       0xa0
//         /* "source":958:975   */
//       sstore
//         /* "source":992:995   */
//       0xc0
//         /* "source":989:990   */
//       0x00
//         /* "source":982:996   */
//       return
//         /* "source":640:1002   */
//       pop
//       pop
//       pop
//       pop
//       pop
//       pop
//     stop
//
//     sub_0: assembly {
//           dataOffset(sub_0)
//             /* "source":1041:1067   */
//           dataSize(sub_0)
//             /* "source":1120:1123   */
//           dup2
//             /* "source":1117:1118   */
//           0x00
//             /* "source":1110:1124   */
//           sstore
//             /* "source":1144:1147   */
//           dup1
//             /* "source":1140:1142   */
//           0x20
//             /* "source":1133:1148   */
//           sstore
//             /* "source":1167:1169   */
//           0x40
//             /* "source":1164:1165   */
//           0x00
//             /* "source":1157:1170   */
//           return
//             /* "source":1031:1178   */
//           pop
//           pop
//         stop
//
//         sub_0: assembly {
//                 /* "source":1223:1232   */
//               invalid
//         }
//     }
//
//     sub_1: assembly {
//             /* "source":1295:1304   */
//           invalid
//     }
// }
// Bytecode: 600060ad604f604760986015609760016096600189600055886020558760405586606055856080558460a0558360c0558260e055816101005580610120556101406000f350505050505050505050fe60306015604560016046600185600055846020558360405582606055816080558060a05560c06000f3505050505050fe60146001816000558060205560406000f35050fefefefefefe60146001816000558060205560406000f35050fefe
// Opcodes: PUSH1 0x0 PUSH1 0xAD PUSH1 0x4F PUSH1 0x47 PUSH1 0x98 PUSH1 0x15 PUSH1 0x97 PUSH1 0x1 PUSH1 0x96 PUSH1 0x1 DUP10 PUSH1 0x0 SSTORE DUP9 PUSH1 0x20 SSTORE DUP8 PUSH1 0x40 SSTORE DUP7 PUSH1 0x60 SSTORE DUP6 PUSH1 0x80 SSTORE DUP5 PUSH1 0xA0 SSTORE DUP4 PUSH1 0xC0 SSTORE DUP3 PUSH1 0xE0 SSTORE DUP2 PUSH2 0x100 SSTORE DUP1 PUSH2 0x120 SSTORE PUSH2 0x140 PUSH1 0x0 RETURN POP POP POP POP POP POP POP POP POP POP INVALID PUSH1 0x30 PUSH1 0x15 PUSH1 0x45 PUSH1 0x1 PUSH1 0x46 PUSH1 0x1 DUP6 PUSH1 0x0 SSTORE DUP5 PUSH1 0x20 SSTORE DUP4 PUSH1 0x40 SSTORE DUP3 PUSH1 0x60 SSTORE DUP2 PUSH1 0x80 SSTORE DUP1 PUSH1 0xA0 SSTORE PUSH1 0xC0 PUSH1 0x0 RETURN POP POP POP POP POP POP INVALID PUSH1 0x14 PUSH1 0x1 DUP2 PUSH1 0x0 SSTORE DUP1 PUSH1 0x20 SSTORE PUSH1 0x40 PUSH1 0x0 RETURN POP POP INVALID INVALID INVALID INVALID INVALID INVALID PUSH1 0x14 PUSH1 0x1 DUP2 PUSH1 0x0 SSTORE DUP1 PUSH1 0x20 SSTORE PUSH1 0x40 PUSH1 0x0 RETURN POP POP INVALID INVALID
// SourceMappings: :::-:0;26:26:0;57:24;86:26;117:24;146:29;180:27;212:29;246:27;278:32;361:3;358:1;351:14;381:3;377:2;370:15;401:3;397:2;390:15;421:3;417:2;410:15;442:4;437:3;430:17;464:4;459:3;452:17;486:4;481:3;474:17;508:4;503:3;496:17;530:5;525:3;518:18;553:5;548:3;541:18;574:3;571:1;564:14;20:562;;;;;;;;;
