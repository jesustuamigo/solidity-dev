contract C {
    function f() pure public {
        require(false, "1234567890123456789012345678901");
        require(false, "12345678901234567890123456789012");
        require(false, "123456789012345678901234567890123");
        require(false, "1234567890123456789012345678901234");
        require(false, "12345678901234567890123456789012345");
        require(false, "123456789012345678901234567890123456");
        require(false, "123456789012345678901234567890121234567890123456789012345678901");
        require(false, "1234567890123456789012345678901212345678901234567890123456789012");
        require(false, "12345678901234567890123456789012123456789012345678901234567890123");
    }
}
// ----
// creation:
//   codeDepositCost: 387600
//   executionCost: 424
//   totalCost: 388024
// external:
//   f(): 428
