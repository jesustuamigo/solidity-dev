contract C {
    fallback() external returns (bytes memory, bytes memory) {}
}
// ----
// TypeError 5570: (45-73): Fallback function can only have a single "bytes memory" return value.
