object "a" {
    code {
        pop(extcall(address(), 0, 0, 0))
        pop(extdelegatecall(address(), 0, 0))
        pop(extstaticcall(address(), 0, 0))
    }
}

// ====
// bytecodeFormat: legacy
// ----
// TypeError 4328: (36-43): The "extcall" instruction is only available on EOF.
// TypeError 3950: (36-63): Expected expression to evaluate to one value, but got 0 values instead.
// TypeError 4328: (77-92): The "extdelegatecall" instruction is only available on EOF.
// TypeError 3950: (77-109): Expected expression to evaluate to one value, but got 0 values instead.
// TypeError 4328: (123-136): The "extstaticcall" instruction is only available on EOF.
// TypeError 3950: (123-153): Expected expression to evaluate to one value, but got 0 values instead.
