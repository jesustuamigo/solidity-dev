{
    let a:u256
    function f() {
        let b:u256
        function g() { let c:u256 }
        let d:u256
    }
}
// ====
// step: functionHoister
// yul: true
// ----
// {
//     let a:u256
//     function g()
//     { let c:u256 }
//     function f()
//     {
//         let b:u256
//         let d:u256
//     }
// }
