contract A {
	receive() external payable {
		revert("no_receive");
	}
}

contract C {
	A a = new A();
	receive() external payable {}
	function f() public {
		address(a).transfer(1 wei);
	}
	function h() public {
		address(a).transfer(100 ether);
	}
	function g() public view returns (uint) {
		return address(this).balance;
	}
}
// ====
// EVMVersion: >=byzantium
// revertStrings: debug
// ----
// (), 10 wei ->
// g() -> 10
// f() -> FAILURE, hex"08c379a0", 0x20, 10, "no_receive"
// h() -> FAILURE
