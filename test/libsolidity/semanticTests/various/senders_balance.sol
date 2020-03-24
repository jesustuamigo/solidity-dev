contract C {
    function f() public view returns (uint256) {
        return msg.sender.balance;
    }
}


contract D {
    C c = new C();

    constructor() public payable {}

    function f() public view returns (uint256) {
        return c.f();
    }
}

// ----
// constructor(), 27 wei ->
// f() -> 27
