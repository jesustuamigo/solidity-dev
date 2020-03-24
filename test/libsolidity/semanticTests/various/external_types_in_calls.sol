contract C1 {
    C1 public bla;

    constructor(C1 x) public {
        bla = x;
    }
}


contract C {
    function test() public returns (C1 x, C1 y) {
        C1 c = new C1(C1(9));
        x = c.bla();
        y = this.t1(C1(7));
    }

    function t1(C1 a) public returns (C1) {
        return a;
    }

    function t2() public returns (C1) {
        return C1(9);
    }
}

// ----
// test() -> 9, 7
// t2() -> 9
