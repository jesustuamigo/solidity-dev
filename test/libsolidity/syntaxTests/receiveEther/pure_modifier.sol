contract C {
    uint x;
    receive() external pure { x = 2; }
}
// ----
// DeclarationError 7793: (29-63): Receive ether function must be payable, but is "pure".
