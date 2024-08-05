contract D { constructor() payable {} }
contract C {
	uint public x;
	constructor() payable {}
	function f(uint amount) public returns (D) {
		x++;
		return (new D){value: amount}();
	}
	function stack(uint depth) public payable returns (address) {
		if (depth > 0)
			return this.stack(depth - 1);
		else
			return address(f(0));
	}
}
// ====
// EVMVersion: >=byzantium
// ----
// constructor(), 20 wei
// gas irOptimized: 61548
// gas irOptimized code: 104600
// gas legacy: 70147
// gas legacy code: 215400
// gas legacyOptimized: 61715
// gas legacyOptimized code: 106800
// f(uint256): 20 -> 0x137aa4dfc0911524504fcd4d98501f179bc13b4a
// x() -> 1
// f(uint256): 20 -> FAILURE
// x() -> 1
// stack(uint256): 1023 -> FAILURE
// gas irOptimized: 252410
// gas legacy: 476845
// gas legacyOptimized: 299061
// x() -> 1
// stack(uint256): 10 -> 0x87948bd7ebbe13a00bfd930c93e4828ab18e3908
// x() -> 2
