contract test {
    /// @notice Multiplies `a` by 7
    function mul(uint a) public returns(uint d) { return a * 7; }
}

// ----
// ----
// :test userdoc
// {
//     "methods":
//     {
//         "mul(uint256)":
//         {
//             "notice": "Multiplies `a` by 7"
//         }
//     }
// }
