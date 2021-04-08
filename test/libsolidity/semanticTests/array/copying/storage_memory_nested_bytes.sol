pragma abicoder               v2;
contract C {
    bytes[] a;

    function f() public returns (bytes[] memory) {
        a.push("abc");
        a.push("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVXYZ");
        bytes[] memory m = a;
        return m;
    }
}
// ====
// compileViaYul: also
// ----
// f() -> 0x20, 0x02, 0x40, 0x80, 3, 0x6162630000000000000000000000000000000000000000000000000000000000, 0x99, 44048183304486788312148433451363384677562265908331949128489393215789685032262, 32241931068525137014058842823026578386641954854143559838526554899205067598957, 49951309422467613961193228765530489307475214998374779756599339590522149884499, 0x54555658595a6162636465666768696a6b6c6d6e6f707172737475767778797a, 0x4142434445464748494a4b4c4d4e4f5051525354555658595a00000000000000
// gas irOptimized: 198384
// gas legacy: 199159
// gas legacyOptimized: 198132
