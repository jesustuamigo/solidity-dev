{
	let x := add(mul(calldataload(2), calldataload(4)), mul(2, calldatasize()))
	let b := x
}
// ----
// rematerialiser
// {
//     let x := add(mul(calldataload(2), calldataload(4)), mul(2, calldatasize()))
//     let b := x
// }
