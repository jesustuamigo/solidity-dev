contract C {
    int256 public a = -0x4200 >> 8;
}
// ====
// compileViaYul: also
// ----
// a() -> -66
