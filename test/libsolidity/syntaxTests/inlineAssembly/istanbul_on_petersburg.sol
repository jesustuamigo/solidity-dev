contract C {
    function f() pure external returns (uint id) {
        assembly {
            id := chainid()
        }
    }
    function g() view external returns (uint sb) {
        assembly {
            sb := selfbalance()
        }
    }
}
// ====
// EVMVersion: =petersburg
// ----
// TypeError: (101-108): The "chainid" instruction is only available for Istanbul-compatible VMs  (you are currently compiling for "petersburg").
// DeclarationError: (95-110): Variable count does not match number of values (1 vs. 0)
// TypeError: (215-226): The "selfbalance" instruction is only available for Istanbul-compatible VMs  (you are currently compiling for "petersburg").
// DeclarationError: (209-228): Variable count does not match number of values (1 vs. 0)
