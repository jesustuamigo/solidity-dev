{
    function f() -> x { pop(address()) let y := callvalue() }
}
// ====
// stackOptimization: true
// ----
// PUSH1 0xD
// JUMP
// JUMPDEST
// PUSH1 0x0
// ADDRESS
// POP
// CALLVALUE
// POP
// JUMPDEST
// SWAP1
// JUMP
// JUMPDEST
