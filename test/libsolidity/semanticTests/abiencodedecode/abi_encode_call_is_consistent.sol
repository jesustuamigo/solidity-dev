pragma abicoder v2;

contract C {
	bool sideEffectRan = false;

	function(uint256, string memory) external fPointer;
	function fExternal(uint256 p, string memory t) external {}
	string xstor;
	function getExternalFunctionPointer() public returns (function(uint256, string memory) external) {
		sideEffectRan = true;
		return this.fExternal;
	}

	function fSignatureFromLiteral() public pure returns (bytes memory) {
		return abi.encodeWithSignature("fExternal(uint256,string)", 1, "123");
	}
	function fSignatureFromLiteralCall() public view returns (bytes memory) {
		return abi.encodeCall(this.fExternal, (1, "123"));
	}
	function fSignatureFromMemory() public pure returns (bytes memory) {
		string memory x = "fExternal(uint256,string)";
		return abi.encodeWithSignature(x, 1, "123");
	}
	function fSignatureFromMemoryCall() public view returns (bytes memory) {
		return abi.encodeCall(this.fExternal, (1,"123"));
	}
	function fSignatureFromMemorys() public returns (bytes memory) {
		xstor = "fExternal(uint256,string)";
		return abi.encodeWithSignature(xstor, 1, "123");
	}
	function fPointerCall() public returns(bytes memory) {
		fPointer = this.fExternal;
		return abi.encodeCall(fPointer, (1, "123"));
	}
	function fLocalPointerCall() public returns(bytes memory) {
		function(uint256, string memory) external localFunctionPointer = this.fExternal;
		return abi.encodeCall(localFunctionPointer, (1, "123"));
	}
	function fReturnedFunctionPointer() public returns (bytes memory) {
		return abi.encodeCall(getExternalFunctionPointer(), (1, "123"));
	}

	function assertConsistentSelectors() public {
		assert(keccak256(fSignatureFromLiteral()) == keccak256(fSignatureFromLiteralCall()));
		assert(keccak256(fSignatureFromMemory()) == keccak256(fSignatureFromMemoryCall()));
		assert(keccak256(fSignatureFromMemoryCall()) == keccak256(fSignatureFromMemorys()));
		assert(keccak256(fPointerCall()) == keccak256(fSignatureFromLiteral()));
		assert(keccak256(fLocalPointerCall()) == keccak256(fSignatureFromLiteral()));
		assert(keccak256(fReturnedFunctionPointer()) == keccak256(fSignatureFromLiteral()));
	}
}
// ====
// compileViaYul: also
// ----
// assertConsistentSelectors() ->
// fSignatureFromLiteral() -> 0x20, 0x84, 23450202028776381066253055403048136312616272755117076566855971503345107992576, 26959946667150639794667015087019630673637144422540572481103610249216, 1725436586697640946858688965569256363112777243042596638790631055949824, 86060793054017993816230018372407419485142305772921726565498526629888, 0
// fSignatureFromLiteralCall() -> 0x20, 0x84, 23450202028776381066253055403048136312616272755117076566855971503345107992576, 26959946667150639794667015087019630673637144422540572481103610249216, 1725436586697640946858688965569256363112777243042596638790631055949824, 86060793054017993816230018372407419485142305772921726565498526629888, 0
// fSignatureFromMemory() -> 0x20, 0x84, 23450202028776381066253055403048136312616272755117076566855971503345107992576, 26959946667150639794667015087019630673637144422540572481103610249216, 1725436586697640946858688965569256363112777243042596638790631055949824, 86060793054017993816230018372407419485142305772921726565498526629888, 0
// fSignatureFromMemoryCall() -> 0x20, 0x84, 23450202028776381066253055403048136312616272755117076566855971503345107992576, 26959946667150639794667015087019630673637144422540572481103610249216, 1725436586697640946858688965569256363112777243042596638790631055949824, 86060793054017993816230018372407419485142305772921726565498526629888, 0
// fSignatureFromMemorys() -> 0x20, 0x84, 23450202028776381066253055403048136312616272755117076566855971503345107992576, 26959946667150639794667015087019630673637144422540572481103610249216, 1725436586697640946858688965569256363112777243042596638790631055949824, 86060793054017993816230018372407419485142305772921726565498526629888, 0
// fPointerCall() -> 0x20, 0x84, 23450202028776381066253055403048136312616272755117076566855971503345107992576, 26959946667150639794667015087019630673637144422540572481103610249216, 1725436586697640946858688965569256363112777243042596638790631055949824, 86060793054017993816230018372407419485142305772921726565498526629888, 0
// fLocalPointerCall() -> 0x20, 0x84, 23450202028776381066253055403048136312616272755117076566855971503345107992576, 26959946667150639794667015087019630673637144422540572481103610249216, 1725436586697640946858688965569256363112777243042596638790631055949824, 86060793054017993816230018372407419485142305772921726565498526629888, 0
// fReturnedFunctionPointer() -> 0x20, 0x84, 23450202028776381066253055403048136312616272755117076566855971503345107992576, 26959946667150639794667015087019630673637144422540572481103610249216, 1725436586697640946858688965569256363112777243042596638790631055949824, 86060793054017993816230018372407419485142305772921726565498526629888, 0
