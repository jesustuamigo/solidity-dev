contract C {
  struct I {
    uint b;
    uint c;
  }
  struct S {
    I a;
  }

  function f() external returns (uint) {
    S memory s = S(I(1,2));
    return s.a.b;
  }
}
// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// f() -> 1
