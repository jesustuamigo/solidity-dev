#pragma once

#include <libdevcore/FixedHash.h>
#include <libdevcore/Keccak256.h>
#include <libdevcore/Whiskers.h>
#include <test/tools/ossfuzz/abiV2Proto.pb.h>
#include <boost/algorithm/string.hpp>
#include <ostream>
#include <sstream>

/**
 * Template of the solidity test program generated by this converter is as follows:
 *
 *  pragma solidity >=0.0;
 *  pragma experimental ABIEncoderV2;
 *
 * contract C {
 *      // State variable
 *      string x_0;
 *      // Test function that is called by the VM.
 *      function test() public returns (uint) {
 *          // Local variable
 *          bytes x_1 = "1";
 *          x_0 = "044852b2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116d";
 *          uint returnVal = this.coder_public(x_0, x_1);
 *          if (returnVal != 0)
 *              return returnVal;
 *          // Since the return codes in the public and external coder functions are identical
 *          // we offset error code by a fixed amount (200000) for differentiation.
 *          returnVal = this.coder_external(x_0, x_1);
 *          if (returnVal != 0)
 *              return 200000 + returnVal;
 *          // Encode parameters
 *          bytes memory argumentEncoding = abi.encode(<parameter_names>);
 *          returnVal = checkEncodedCall(this.coder_public.selector, argumentEncoding, <invalidLengthFuzz>);
 *          // Check if calls to coder_public meet expectations for correctly/incorrectly encoded data.
 *          if (returnVal != 0)
 *              return returnVal;
 *
 *          returnVal = checkEncodedCall(this.coder_external.selector, argumentEncoding, <invalidLengthFuzz>);
 *          // Check if calls to coder_external meet expectations for correctly/incorrectly encoded data.
 *          // Offset return value to distinguish between failures originating from coder_public and coder_external.
 *          if (returnVal != 0)
 *              return uint(200000) + returnVal;
 *          // Return zero if all checks pass.
 *          return 0;
 *      }
 *
 *      /// Accepts function selector, correct argument encoding, and an invalid encoding length as input.
 *      /// Returns a non-zero value if either call with correct encoding fails or call with incorrect encoding
 *      /// succeeds. Returns zero if both calls meet expectation.
 *      function checkEncodedCall(bytes4 funcSelector, bytes memory argumentEncoding, uint invalidLengthFuzz)
 *          public returns (uint) {
 *          ...
 *      }
 *
 *      /// Accepts function selector, correct argument encoding, and length of invalid encoding and returns
 *      /// the correct and incorrect abi encoding for calling the function specified by the function selector.
 *      function createEncoding(bytes4 funcSelector, bytes memory argumentEncoding, uint invalidLengthFuzz)
 *          internal pure returns (bytes memory, bytes memory) {
 *          ...
 *      }
 *
 *      /// Compares two dynamically sized bytes arrays for equality.
 *      function bytesCompare(bytes memory a, bytes memory b) internal pure returns (bool) {
 *          ...
 *      }
 *
 *      // Public function that is called by test() function. Accepts one or more arguments and returns
 *      // a uint value (zero if abi en/decoding was successful, non-zero otherwise)
 *      function coder_public(string memory c_0, bytes memory c_1) public pure returns (uint) {
 *              if (!bytesCompare(bytes(c_0), "044852b2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116d"))
 *	    	        return 1;
 *              if (!bytesCompare(c_1, "1"))
 *                  return 2;
 *              return 0;
 *	    }
 *
 *      // External function that is called by test() function. Accepts one or more arguments and returns
 *      // a uint value (zero if abi en/decoding was successful, non-zero otherwise)
 *      function coder_external(string calldata c_0, bytes calldata c_1) external pure returns (uint) {
 *              if (!bytesCompare(bytes(c_0), "044852b2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116d"))
 *  		        return 1;
 *              if (!bytesCompare(c_1, "1"))
 *                  return 2;
 *              return 0;
 *	    }
 * }
 */


namespace dev
{
namespace test
{
namespace abiv2fuzzer
{
class ProtoConverter
{
public:
	ProtoConverter():
		m_isStateVar(true),
		m_counter(0),
		m_varCounter(0),
		m_returnValue(1)
	{}

	ProtoConverter(ProtoConverter const&) = delete;

	ProtoConverter(ProtoConverter&&) = delete;

	std::string contractToString(Contract const& _input);

private:
	using VecOfBoolUnsigned = std::vector<std::pair<bool, unsigned>>;

	enum class Delimiter
	{
		ADD,
		SKIP
	};
	enum class CalleeType
	{
		PUBLIC,
		EXTERNAL
	};
	enum class DataType
	{
		BYTES,
		STRING,
		VALUE,
		ARRAY
	};

	void visit(BoolType const&);

	void visit(IntegerType const&);

	void visit(FixedByteType const&);

	void visit(AddressType const&);

	void visit(ArrayType const&);

	void visit(DynamicByteArrayType const&);

	void visit(StructType const&);

	void visit(ValueType const&);

	void visit(NonValueType const&);

	void visit(Type const&);

	void visit(VarDecl const&);

	void visit(TestFunction const&);

	void visit(Contract const&);

	std::string getValueByBaseType(ArrayType const&);

	DataType getDataTypeByBaseType(ArrayType const& _x);

	void resizeInitArray(
		ArrayType const& _x,
		std::string const& _baseType,
		std::string const& _varName,
		std::string const& _paramName
	);

	unsigned resizeDimension(
		bool _isStatic,
		unsigned _arrayLen,
		std::string const& _var,
		std::string const& _param,
		std::string const& _type
	);

	void resizeHelper(
		ArrayType const& _x,
		std::vector<std::string> _arrStrVec,
		VecOfBoolUnsigned _arrInfoVec,
		std::string const& _varName,
		std::string const& _paramName
	);

	// Utility functions
	void appendChecks(DataType _type, std::string const& _varName, std::string const& _rhs);

	void addVarDef(std::string const& _varName, std::string const& _rhs);

	void addCheckedVarDef(
		DataType _type,
		std::string const& _varName,
		std::string const& _paramName,
		std::string const& _rhs
	);

	void appendTypedParams(
		CalleeType _calleeType,
		bool _isValueType,
		std::string const& _typeString,
		std::string const& _varName,
		Delimiter _delimiter
	);

	void appendTypedParamsPublic(
		bool _isValueType,
		std::string const& _typeString,
		std::string const& _varName,
		Delimiter _delimiter = Delimiter::ADD
	);

	void appendTypedParamsExternal(
		bool _isValueType,
		std::string const& _typeString,
		std::string const& _varName,
		Delimiter _delimiter = Delimiter::ADD
	);

	void appendVarDeclToOutput(
		std::string const& _type,
		std::string const& _varName,
		std::string const& _qualifier
	);

	void checkResizeOp(std::string const& _varName, unsigned _len);

	void visitType(DataType _dataType, std::string const& _type, std::string const& _value);

	void visitArrayType(std::string const&, ArrayType const&);

	void createDeclAndParamList(
		std::string const& _type,
		DataType _dataType,
		std::string& _varName,
		std::string& _paramName
	);

	std::string equalityChecksAsString();

	std::string typedParametersAsString(CalleeType _calleeType);

	void writeHelperFunctions();

	// Function definitions
	// m_counter is used to derive values for typed variables
	unsigned getNextCounter()
	{
		return m_counter++;
	}

	// m_varCounter is used to derive declared and parameterized variable names
	unsigned getNextVarCounter()
	{
		return m_varCounter++;
	}

	// Accepts an unsigned counter and returns a pair of strings
	// First string is declared name (s_varNamePrefix<varcounter_value>)
	// Second string is parameterized name (s_paramPrefix<varcounter_value>)
	auto newVarNames(unsigned _varCounter)
	{
		return std::make_pair(
			s_varNamePrefix + std::to_string(_varCounter),
			s_paramNamePrefix + std::to_string(_varCounter)
		);
	}

	std::string getQualifier(DataType _dataType)
	{
		return ((isValueType(_dataType) || m_isStateVar) ? "" : "memory");
	}

	// Static declarations
	static std::string structTypeAsString(StructType const& _x);
	static std::string boolValueAsString(unsigned _counter);
	static std::string intValueAsString(unsigned _width, unsigned _counter);
	static std::string uintValueAsString(unsigned _width, unsigned _counter);
	static std::string integerValueAsString(bool _sign, unsigned _width, unsigned _counter);
	static std::string addressValueAsString(unsigned _counter);
	static std::string fixedByteValueAsString(unsigned _width, unsigned _counter);
	static std::string hexValueAsString(
		unsigned _width,
		unsigned _counter,
		bool _isHexLiteral
	);
	static std::vector<std::pair<bool, unsigned>> arrayDimensionsAsPairVector(ArrayType const& _x);
	static std::string arrayDimInfoAsString(ArrayDimensionInfo const& _x);
	static void arrayDimensionsAsStringVector(
		ArrayType const& _x,
		std::vector<std::string>&
	);
	static std::string bytesArrayTypeAsString(DynamicByteArrayType const& _x);
	static std::string arrayTypeAsString(std::string const&, ArrayType const&);
	static std::string delimiterToString(Delimiter _delimiter);

	// Static function definitions
	static bool isValueType(DataType _dataType)
	{
		return _dataType == DataType::VALUE;
	}

	static unsigned getIntWidth(IntegerType const& _x)
	{
		return 8 * ((_x.width() % 32) + 1);
	}

	static bool isIntSigned(IntegerType const& _x)
	{
		return _x.is_signed();
	}

	static std::string getBoolTypeAsString()
	{
		return "bool";
	}

	static std::string getIntTypeAsString(IntegerType const& _x)
	{
		return ((isIntSigned(_x) ? "int" : "uint") + std::to_string(getIntWidth(_x)));
	}

	static unsigned getFixedByteWidth(FixedByteType const& _x)
	{
		return (_x.width() % 32) + 1;
	}

	static std::string getFixedByteTypeAsString(FixedByteType const& _x)
	{
		return "bytes" + std::to_string(getFixedByteWidth(_x));
	}

	static std::string getAddressTypeAsString(AddressType const& _x)
	{
		return (_x.payable() ? "address payable": "address");
	}

	static DataType getDataTypeOfDynBytesType(DynamicByteArrayType const& _x)
	{
		if (_x.type() == DynamicByteArrayType::STRING)
			return DataType::STRING;
		else
			return DataType::BYTES;
	}

	// Convert _counter to string and return its keccak256 hash
	static u256 hashUnsignedInt(unsigned _counter)
	{
		return keccak256(h256(_counter));
	}

	static u256 maskUnsignedInt(unsigned _counter, unsigned _numMaskNibbles)
	{
	  return hashUnsignedInt(_counter) & u256("0x" + std::string(_numMaskNibbles, 'f'));
	}

	// Requires caller to pass number of nibbles (twice the number of bytes) as second argument.
	// Note: Don't change HexPrefix::Add. See comment in fixedByteValueAsString().
	static std::string maskUnsignedIntToHex(unsigned _counter, unsigned _numMaskNibbles)
	{
		return toHex(maskUnsignedInt(_counter, _numMaskNibbles), HexPrefix::Add);
	}

	/// Dynamically sized arrays can have a length of at least zero
	/// and at most s_maxArrayLength.
	static unsigned getDynArrayLengthFromFuzz(unsigned _fuzz, unsigned _counter)
	{
		// Increment modulo value by one in order to meet upper bound
		return (_fuzz + _counter) % (s_maxArrayLength + 1);
	}

	/// Statically sized arrays must have a length of at least one
	/// and at most s_maxArrayLength.
	static unsigned getStaticArrayLengthFromFuzz(unsigned _fuzz)
	{
		return _fuzz % s_maxArrayLength + 1;
	}

	static std::pair<bool, unsigned> arrayDimInfoAsPair(ArrayDimensionInfo const& _x)
	{
		return (
			_x.is_static() ?
			std::make_pair(true, getStaticArrayLengthFromFuzz(_x.length())) :
			std::make_pair(false, getDynArrayLengthFromFuzz(_x.length(), 0))
		);
	}

	// String and bytes literals are derived by hashing a monotonically increasing
	// counter and enclosing the (potentially cropped) hash inside double quotes.
	// Cropping is achieved by masking out higher order bits.
	// TODO: Test invalid encoding of bytes/string arguments that hold values of over 32 bytes.
	// See https://github.com/ethereum/solidity/issues/7180
	static std::string bytesArrayValueAsString(unsigned _counter, bool _isHexLiteral)
	{
		// We use _counter to not only create a value but to crop it
		// to a length (l) such that 0 <= l <= 32 (hence the use of 33 as
		// the modulo constant)
		return hexValueAsString(_counter % 33, _counter, _isHexLiteral);
	}

	/// Contains the test program
	std::ostringstream m_output;
	/// Temporary storage for state variable definitions
	std::ostringstream m_statebuffer;
	/// Contains a subset of the test program. This subset contains
	/// checks to be encoded in the test program
	std::ostringstream m_checks;
	/// Contains typed parameter list to be passed to callee functions
	std::ostringstream m_typedParamsExternal;
	std::ostringstream m_typedParamsPublic;
	/// Predicate that is true if we are in contract scope
	bool m_isStateVar;
	unsigned m_counter;
	unsigned m_varCounter;
	/// Monotonically increasing return value for error reporting
	unsigned m_returnValue;
	static unsigned constexpr s_maxArrayLength = 4;
	static unsigned constexpr s_maxArrayDimensions = 4;
	/// Prefixes for declared and parameterized variable names
	static auto constexpr s_varNamePrefix = "x_";
	static auto constexpr s_paramNamePrefix = "c_";
};
}
}
}
