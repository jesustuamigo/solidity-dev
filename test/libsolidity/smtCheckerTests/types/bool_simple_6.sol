contract C {
    function f(bool x) public pure {
        require(x);
        bool y;
        y = false;
        assert(x || y);
    }
}
// ====
// SMTEngine: all
// ----
// Info 1391: CHC: 1 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
