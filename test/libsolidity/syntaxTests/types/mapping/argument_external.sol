contract C {
    function f(mapping(uint => uint) storage) external pure {
    }
}
// ----
// TypeError: (28-57): Data location must be "memory" or "calldata" for parameter in external function, but "storage" was given.
