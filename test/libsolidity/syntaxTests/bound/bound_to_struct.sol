contract C {
    struct S { uint t; }
    function r() public {
        S memory x;
        x.d;
    }
    using S for S;
}
// ----
// TypeError: (113-114): Library name expected.
