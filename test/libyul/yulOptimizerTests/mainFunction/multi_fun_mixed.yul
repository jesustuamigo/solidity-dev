{
    let a:u256
    function f() { let b:u256 }
    let c:u256
    function g() { let d:u256 }
    let e:u256
}
// ====
// step: mainFunction
// dialect: yul
// ----
// {
//     function main()
//     {
//         let a:u256
//         let c:u256
//         let e:u256
//     }
//     function f()
//     { let b:u256 }
//     function g()
//     { let d:u256 }
// }
