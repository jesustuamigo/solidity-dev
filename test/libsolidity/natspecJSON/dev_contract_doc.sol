/// @author Lefteris
/// @title Just a test contract
contract test {
    /// @dev Mul function
    function mul(uint a, uint second) public returns (uint d) { return a * 7 + second; }
}

// ----
// ----
// :test devdoc
// {
//     "author": "Lefteris",
//     "methods":
//     {
//         "mul(uint256,uint256)":
//         {
//             "details": "Mul function"
//         }
//     },
//     "title": "Just a test contract"
// }
