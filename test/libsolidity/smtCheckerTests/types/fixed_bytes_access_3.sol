contract C {
	bytes16[][] a;
	constructor() {
		a.push();
		a.push();
		a.push();
		a.push();
		a.push();
		a.push();
		a[2].push();
		a[2].push();
		a[2].push();
		a[3].push();
		a[3].push();
		a[3].push();
		a[3].push();
		a[3].push();
	}
	function g() internal view returns (bytes16[] storage) {
		return a[2];
	}
	function h() internal view returns (bytes16) {
		return a[2][2];
	}
	function f() external view {
		// Reports oob because of aliasing.
		g()[3][4];
		h()[5];
	}
}
// ====
// SMTEngine: all
// ----
// Warning 6368: (456-462): CHC: Out of bounds access happens here.
