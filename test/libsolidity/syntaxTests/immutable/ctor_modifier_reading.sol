contract C {
    uint immutable x;
    constructor() readX {
        x = 3;
    }

    modifier readX() {
        _; f(x);
    }

    function f(uint a) internal pure {}
}
