{
    let b := 3
    function f(a, r) -> t {
        // r could be removed right away, but a cannot - this is not implemented, though
        let x := a a := 3 t := a
    }
    b := 7
}
// ====
// stackOptimization: true
// ----
// PUSH1 0x3
// PUSH1 0x17
// JUMP
// JUMPDEST
// PUSH1 0x0
// SWAP2
// POP
// DUP1
// POP
// PUSH1 0x3
// SWAP1
// POP
// DUP1
// SWAP2
// POP
// POP
// JUMPDEST
// SWAP1
// JUMP
// JUMPDEST
// PUSH1 0x7
// SWAP1
// POP
// POP
