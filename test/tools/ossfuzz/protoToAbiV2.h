#pragma once

#include <test/tools/ossfuzz/abiV2Proto.pb.h>

#include <libsolutil/FixedHash.h>
#include <libsolutil/Keccak256.h>
#include <libsolutil/StringUtils.h>
#include <libsolutil/Whiskers.h>
#include <libsolutil/Numeric.h>

#include <liblangutil/Exceptions.h>

#include <boost/variant/static_visitor.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>

#include <ostream>
#include <sstream>

/**
 * Template of the solidity test program generated by this converter is as follows:
 *
 *	pragma solidity >=0.0;
 *	pragma experimental ABIEncoderV2;
 *
 *	contract C {
 *		// State variable
 *		string sv_0;
 *		// Test function that is called by the VM.
 *		// There are 2 variations of this function: one returns
 *		// the output of test_calldata_coding() and the other
 *		// returns the output of test_returndata_coding(). The
 *		// proto field called Test decides which one of the two
 *		// are chosen for creating a test case.
 *		function test() public returns (uint) {
 *			// The protobuf field "Contract.test" decides which of
 *			// the two internal functions "test_calldata_coding()"
 *			// and "test_returndata_coding()" are called. Here,
 *			// we assume that the protobuf field equals "CALLDATA_CODER"
 *			return this.test_calldata_coding()
 *		}
 *
 *		// The following function is generated if the protobuf field
 *		// "Contract.test" is equal to "RETURNDATA_CODER".
 *		function test_returndata_coding() internal returns (uint) {
 *			string memory lv_0, bytes memory lv_1 = test_returndata_external();
 *			if (lv_0 != 044852b2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116d)
 *				return 1;
 *			if (lv_1 != "1")
 *				return 2;
 *			return 0;
 *		}
 *
 *		// The following function is generated if the protobuf field
 *		// "Contract.test" is equal to "RETURNDATA_CODER".
 *		function test_returndata_external() external returns (string memory, bytes memory)
 *		{
 *			sv_0 = "044852b2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116d";
 *			bytes memory lv_0 = "1";
 *			return (sv_0, lv_0);
 *		}
 *
  *		// The following function is generated if the protobuf field
 *		// "Contract.test" is equal to "CALLDATA_CODER".
 *		function test_calldata_coding() internal returns (uint) {
 *			// Local variable
 *			bytes lv_1 = "1";
 *			sv_0 = "044852b2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116d";
 *			uint returnVal = this.coder_public(sv_0, lv_1);
 *			if (returnVal != 0)
 *				return returnVal;
 *			// Since the return codes in the public and external coder functions are identical
 *			// we offset error code by a fixed amount (200000) for differentiation.
 *			returnVal = this.coder_external(sv_0, lv_1);
 *			if (returnVal != 0)
 *				return 200000 + returnVal;
 *			// Encode parameters
 *			bytes memory argumentEncoding = abi.encode(<parameter_names>);
 *			returnVal = checkEncodedCall(this.coder_public.selector, argumentEncoding, <invalidLengthFuzz>);
 *			// Check if calls to coder_public meet expectations for correctly/incorrectly encoded data.
 *			if (returnVal != 0)
 *				return returnVal;
 *
 *			returnVal = checkEncodedCall(this.coder_external.selector, argumentEncoding, <invalidLengthFuzz>);
 *			// Check if calls to coder_external meet expectations for correctly/incorrectly encoded data.
 *			// Offset return value to distinguish between failures originating from coder_public and coder_external.
 *			if (returnVal != 0)
 *				return uint(200000) + returnVal;
 *			// Return zero if all checks pass.
 *			return 0;
 *		}
 *
 *		/// Accepts function selector, correct argument encoding, and an invalid encoding length as input.
 *		/// Returns a non-zero value if either call with correct encoding fails or call with incorrect encoding
 *		/// succeeds. Returns zero if both calls meet expectation.
 *		function checkEncodedCall(bytes4 funcSelector, bytes memory argumentEncoding, uint invalidLengthFuzz)
 *			public returns (uint) {
 *			...
 *		}
 *
 *		/// Accepts function selector, correct argument encoding, and length of invalid encoding and returns
 *		/// the correct and incorrect abi encoding for calling the function specified by the function selector.
 *		function createEncoding(bytes4 funcSelector, bytes memory argumentEncoding, uint invalidLengthFuzz)
 *			internal pure returns (bytes memory, bytes memory) {
 *			...
 *		}
 *
 *		/// Compares two dynamically sized bytes arrays for equality.
 *		function bytesCompare(bytes memory a, bytes memory b) internal pure returns (bool) {
 *			...
 *		}
 *
 *		// Public function that is called by test() function. Accepts one or more arguments and returns
 *		// a uint value (zero if abi en/decoding was successful, non-zero otherwise)
 *		function coder_public(string memory c_0, bytes memory c_1) public pure returns (uint) {
 *			if (!bytesCompare(bytes(c_0), "044852b2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116d"))
 *				return 1;
 *			if (!bytesCompare(c_1, "1"))
 *				return 2;
 *			return 0;
 *		}
 *
 *		// External function that is called by test() function. Accepts one or more arguments and returns
 *		// a uint value (zero if abi en/decoding was successful, non-zero otherwise)
 *		function coder_external(string calldata c_0, bytes calldata c_1) external pure returns (uint) {
 *			if (!bytesCompare(bytes(c_0), "044852b2a670ade5407e78fb2863c51de9fcb96542a07186fe3aeda6bb8a116d"))
 *				return 1;
 *			if (!bytesCompare(c_1, "1"))
 *				return 2;
 *			return 0;
 *		}
 *	}
 */

namespace solidity::test::abiv2fuzzer
{

/// Converts a protobuf input into a Solidity program that tests
/// abi coding.
class ProtoConverter
{
public:
	ProtoConverter():
		m_isStateVar(true),
		m_counter(0),
		m_varCounter(0),
		m_returnValue(1),
		m_isLastDynParamRightPadded(false),
		m_structCounter(0),
		m_numStructsAdded(0)
	{}

	ProtoConverter(ProtoConverter const&) = delete;
	ProtoConverter(ProtoConverter&&) = delete;
	std::string contractToString(Contract const& _input);
	std::string isabelleTypeString() const;
	std::string isabelleValueString() const;
	bool coderFunction() const
	{
		return m_test == Contract_Test::Contract_Test_CALLDATA_CODER;
	}
private:
	enum class Delimiter
	{
		ADD,
		SKIP
	};
	/// Enum of possible function types that decode abi
	/// encoded parameters.
	enum class CalleeType
	{
		PUBLIC,
		EXTERNAL
	};

	/// Visitors for various Protobuf types
	/// Visit top-level contract specification
	void visit(Contract const&);

	/// Convert test function specification into Solidity test
	/// function
	/// @param _testSpec: Protobuf test function specification
	/// @param _storageDefs: String containing Solidity assignment
	/// statements to be placed inside the scope of the test function.
	std::string visit(TestFunction const& _testSpec, std::string const& _storageDefs);

	/// Visitors for the remaining protobuf types. They convert
	/// the input protobuf specification type into Solidity code.
	/// @return A pair of strings, first of which contains Solidity
	/// code to be placed inside contract scope, second of which contains
	/// Solidity code to be placed inside test function scope.
	std::pair<std::string, std::string> visit(VarDecl const&);
	std::pair<std::string, std::string> visit(Type const&);
	std::pair<std::string, std::string> visit(ValueType const&);
	std::pair<std::string, std::string> visit(NonValueType const&);
	std::pair<std::string, std::string> visit(BoolType const&);
	std::pair<std::string, std::string> visit(IntegerType const&);
	std::pair<std::string, std::string> visit(FixedByteType const&);
	std::pair<std::string, std::string> visit(AddressType const&);
	std::pair<std::string, std::string> visit(DynamicByteArrayType const&);
	std::pair<std::string, std::string> visit(ArrayType const&);
	std::pair<std::string, std::string> visit(StructType const&);

	/// Convert a protobuf type @a _T into Solidity code to be placed
	/// inside contract and test function scopes.
	/// @param: _type (of parameterized type protobuf type T) is the type
	/// of protobuf input to be converted.
	/// @param: _isValueType is true if _type is a Solidity value type e.g., uint
	/// and false otherwise e.g., string
	/// @return: A pair of strings, first of which contains Solidity
	/// code to be placed inside contract scope, second of which contains
	/// Solidity code to be placed inside test function scope.
	template <typename T>
	std::pair<std::string, std::string> processType(T const& _type, bool _isValueType);

	/// Convert a protobuf type @a _T into Solidity variable assignment and check
	/// statements to be placed inside contract and test function scopes.
	/// @param: _varName is the name of the Solidity variable
	/// @param: _paramName is the name of the Solidity parameter that is passed
	/// to the test function
	/// @param: _type (of parameterized type protobuf type T) is the type
	/// of protobuf input to be converted.
	/// @return: A pair of strings, first of which contains Solidity
	/// statements to be placed inside contract scope, second of which contains
	/// Solidity statements to be placed inside test function scope.
	template <typename T>
	std::pair<std::string, std::string> assignChecker(
		std::string const& _varName,
		std::string const& _paramName,
		T _type
	);

	/// Convert a protobuf type @a _T into Solidity variable declaration statement.
	/// @param: _varName is the name of the Solidity variable
	/// @param: _paramName is the name of the Solidity parameter that is passed
	/// to the test function
	/// @param: _type (of parameterized type protobuf type T) is the type
	/// of protobuf input to be converted.
	/// @param: _isValueType is a boolean that is true if _type is a
	/// Solidity value type e.g., uint and false otherwise e.g., string
	/// @param: _location is the Solidity location qualifier string to
	/// be used inside variable declaration statements
	/// @return: A pair of strings, first of which contains Solidity
	/// variable declaration statement to be placed inside contract scope,
	/// second of which contains Solidity variable declaration statement
	/// to be placed inside test function scope.
	template <typename T>
	std::pair<std::string, std::string> varDecl(
		std::string const& _varName,
		std::string const& _paramName,
		T _type,
		bool _isValueType,
		std::string const& _location
	);

	/// Appends a function parameter to the function parameter stream.
	void appendTypedParams(
		CalleeType _calleeType,
		bool _isValueType,
		std::string const& _typeString,
		std::string const& _varName,
		Delimiter _delimiter
	);

	/// Appends a function parameter to the public test function's
	/// parameter stream.
	void appendTypedParamsPublic(
		bool _isValueType,
		std::string const& _typeString,
		std::string const& _varName,
		Delimiter _delimiter = Delimiter::ADD
	);

	/// Appends a function parameter to the external test function's
	/// parameter stream.
	void appendTypedParamsExternal(
		bool _isValueType,
		std::string const& _typeString,
		std::string const& _varName,
		Delimiter _delimiter = Delimiter::ADD
	);

	/// Append types to typed stream used by returndata coders.
	void appendTypes(
		bool _isValueType,
		std::string const& _typeString,
		Delimiter _delimiter
	);

	/// Append typed return value.
	void appendTypedReturn(
		bool _isValueType,
		std::string const& _typeString,
		Delimiter _delimiter
	);

	/// Append type name to type string meant to be
	/// passed to Isabelle coder API.
	void appendToIsabelleTypeString(
		std::string const& _typeString,
		Delimiter _delimiter
	);

	/// Append @a _valueString to value string meant to be
	/// passed to Isabelle coder API.
	void appendToIsabelleValueString(
		std::string const& _valueString,
		Delimiter _delimiter
	);

	/// Returns a Solidity variable declaration statement
	/// @param _type: string containing Solidity type of the
	/// variable to be declared.
	/// @param _varName: string containing Solidity variable
	/// name
	/// @param _qualifier: string containing location where
	/// the variable will be placed
	std::string getVarDecl(
		std::string const& _type,
		std::string const& _varName,
		std::string const& _qualifier
	);

	/// Return checks that are encoded as Solidity if statements
	/// as string
	std::string equalityChecksAsString();

	/// Return comma separated typed function parameters as string
	std::string typedParametersAsString(CalleeType _calleeType);

	/// Return commonly used Solidity helper functions as string
	std::string commonHelperFunctions();

	/// Return helper functions used to test calldata coding
	std::string calldataHelperFunctions();

	/// Return top-level calldata coder test function as string
	std::string testCallDataFunction(unsigned _invalidLength);

	/// Return top-level returndata coder test function as string
	std::string testReturnDataFunction();

	/// Return the next variable count that is used for
	/// variable naming.
	unsigned getNextVarCounter()
	{
		return m_varCounter++;
	}

	/// Return a pair of names for Solidity variable and the same variable when
	/// passed either as a function parameter or used to store the tuple
	/// returned from a function.
	/// @param _varCounter: name suffix
	/// @param _stateVar: predicate that is true for state variables, false otherwise
	std::pair<std::string, std::string> newVarNames(unsigned _varCounter, bool _stateVar)
	{
		std::string varName = _stateVar ? s_stateVarNamePrefix : s_localVarNamePrefix;
		return std::make_pair(
			varName + std::to_string(_varCounter),
			paramName() + std::to_string(_varCounter)
		);
	}

	std::string paramName()
	{
		switch (m_test)
		{
		case Contract_Test::Contract_Test_CALLDATA_CODER:
			return s_paramNamePrefix;
		case Contract_Test::Contract_Test_RETURNDATA_CODER:
			return s_localVarNamePrefix;
		}
	}

	/// Checks if the last dynamically encoded Solidity type is right
	/// padded, returning true if it is and false otherwise.
	bool isLastDynParamRightPadded()
	{
		return m_isLastDynParamRightPadded;
	}

	/// Convert delimter to a comma or null string.
	static std::string delimiterToString(Delimiter _delimiter, bool _space = true);

	/// Contains the test program
	std::ostringstream m_output;
	/// Contains a subset of the test program. This subset contains
	/// checks to be encoded in the test program
	std::ostringstream m_checks;
	/// Contains typed parameter list to be passed to callee functions
	std::ostringstream m_typedParamsExternal;
	std::ostringstream m_typedParamsPublic;
	/// Contains parameter list to be passed to callee functions
	std::ostringstream m_untypedParamsExternal;
	/// Contains type string to be passed to Isabelle API
	std::ostringstream m_isabelleTypeString;
	/// Contains values to be encoded in the format accepted
	/// by the Isabelle API.
	std::ostringstream m_isabelleValueString;
	/// Contains type stream to be used in returndata coder function
	/// signature
	std::ostringstream m_types;
	std::ostringstream m_typedReturn;
	/// Argument names to be passed to coder functions
	std::ostringstream m_argsCoder;
	/// Predicate that is true if we are in contract scope
	bool m_isStateVar;
	unsigned m_counter;
	unsigned m_varCounter;
	/// Monotonically increasing return value for error reporting
	unsigned m_returnValue;
	/// Flag that indicates if last dynamically encoded parameter
	/// passed to a function call is of a type that is going to be
	/// right padded by the ABI encoder.
	bool m_isLastDynParamRightPadded;
	/// Struct counter
	unsigned m_structCounter;
	unsigned m_numStructsAdded;
	/// Enum stating abiv2 coder to be tested
	Contract_Test m_test;
	/// Prefixes for declared and parameterized variable names
	static auto constexpr s_localVarNamePrefix = "lv_";
	static auto constexpr s_stateVarNamePrefix = "sv_";
	static auto constexpr s_paramNamePrefix = "p_";
};

/// Visitor interface for Solidity protobuf types.
template <typename T>
class AbiV2ProtoVisitor
{
public:
	static unsigned constexpr s_maxArrayDimensions = 3;
	virtual ~AbiV2ProtoVisitor() = default;

	virtual T visit(BoolType const& _node) = 0;
	virtual T visit(IntegerType const& _node) = 0;
	virtual T visit(FixedByteType const& _node) = 0;
	virtual T visit(AddressType const& _node) = 0;
	virtual T visit(DynamicByteArrayType const& _node) = 0;
	virtual T visit(ArrayType const& _node) = 0;
	virtual T visit(StructType const& _node) = 0;
	virtual T visit(ValueType const& _node)
	{
		return visitValueType(_node);
	}
	virtual T visit(NonValueType const& _node)
	{
		return visitNonValueType(_node);
	}
	virtual T visit(Type const& _node)
	{
		return visitType(_node);
	}

	enum class DataType
	{
		BYTES,
		VALUE,
		ARRAY
	};

	/// Prefixes for declared and parameterized variable names
	static auto constexpr s_structNamePrefix = "S";

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

	// Convert _counter to string and return its keccak256 hash
	static u256 hashUnsignedInt(unsigned _counter)
	{
		return util::keccak256(util::h256(_counter));
	}

	static u256 maskUnsignedInt(unsigned _counter, unsigned _numMaskNibbles)
	{
		return hashUnsignedInt(_counter) & u256("0x" + std::string(_numMaskNibbles, 'f'));
	}

	// Requires caller to pass number of nibbles (twice the number of bytes) as second argument.
	// Note: Don't change HexPrefix::Add. See comment in fixedByteValueAsString().
	static std::string maskUnsignedIntToHex(unsigned _counter, unsigned _numMaskNibbles)
	{
		return "0x" + toHex(maskUnsignedInt(_counter, _numMaskNibbles));
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

	/// Returns a pseudo-random value for the size of a string/hex
	/// literal. Used for creating variable length hex/string literals.
	/// @param _counter Monotonically increasing counter value
	static unsigned getVarLength(unsigned _counter)
	{
		// Since _counter values are usually small, we use
		// this linear equation to make the number derived from
		// _counter approach a uniform distribution over
		// [0, s_maxDynArrayLength]
		auto v = (_counter + 879) * 32 % (s_maxDynArrayLength + 1);
		/// Always return an even number because Isabelle string
		/// values are formatted as hex literals
		if (v % 2 == 1)
			return v + 1;
		else
			return v;
	}
protected:
T visitValueType(ValueType const& _type)
{
	switch (_type.value_type_oneof_case())
	{
	case ValueType::kInty:
		return visit(_type.inty());
	case ValueType::kByty:
		return visit(_type.byty());
	case ValueType::kAdty:
		return visit(_type.adty());
	case ValueType::kBoolty:
		return visit(_type.boolty());
	case ValueType::VALUE_TYPE_ONEOF_NOT_SET:
		return T();
	}
}

T visitNonValueType(NonValueType const& _type)
{
	switch (_type.nonvalue_type_oneof_case())
	{
	case NonValueType::kDynbytearray:
		return visit(_type.dynbytearray());
	case NonValueType::kArrtype:
		return visit(_type.arrtype());
	case NonValueType::kStype:
		return visit(_type.stype());
	case NonValueType::NONVALUE_TYPE_ONEOF_NOT_SET:
		return T();
	}
}

T visitType(Type const& _type)
{
	switch (_type.type_oneof_case())
	{
	case Type::kVtype:
		return visit(_type.vtype());
	case Type::kNvtype:
		return visit(_type.nvtype());
	case Type::TYPE_ONEOF_NOT_SET:
		return T();
	}
}
private:
	static unsigned constexpr s_maxArrayLength = 4;
	static unsigned constexpr s_maxDynArrayLength = 256;
};

/// Converts a protobuf type into a Solidity type string.
class TypeVisitor: public AbiV2ProtoVisitor<std::string>
{
public:
	TypeVisitor(unsigned _structSuffix = 0):
		m_indentation(1),
		m_structCounter(_structSuffix),
		m_structStartCounter(_structSuffix),
		m_structFieldCounter(0),
		m_isLastDynParamRightPadded(false)
	{}

	std::string visit(BoolType const&) override;
	std::string visit(IntegerType const&) override;
	std::string visit(FixedByteType const&) override;
	std::string visit(AddressType const&) override;
	std::string visit(ArrayType const&) override;
	std::string visit(DynamicByteArrayType const&) override;
	std::string visit(StructType const&) override;
	using AbiV2ProtoVisitor<std::string>::visit;
	std::string baseType()
	{
		return m_baseType;
	}
	bool isLastDynParamRightPadded()
	{
		return m_isLastDynParamRightPadded;
	}
	std::string structDef()
	{
		return m_structDef.str();
	}
	unsigned numStructs()
	{
		return m_structCounter - m_structStartCounter;
	}
	static bool arrayOfStruct(ArrayType const& _type)
	{
		Type const& baseType = _type.t();
		if (baseType.has_nvtype() && baseType.nvtype().has_stype())
			return true;
		else if (baseType.has_nvtype() && baseType.nvtype().has_arrtype())
			return arrayOfStruct(baseType.nvtype().arrtype());
		else
			return false;
	}
	std::string isabelleTypeString()
	{
		return m_structTupleString.stream.str();
	}
private:
	struct StructTupleString
	{
		StructTupleString() = default;
		unsigned index = 0;
		std::ostringstream stream;
		void start()
		{
			stream << "(";
		}
		void end()
		{
			stream << ")";
		}
		void addTypeStringToTuple(std::string& _typeString);
		void addArrayBracketToType(std::string& _arrayBracket);
	};
	void structDefinition(StructType const&);

	std::string indentation()
	{
		return std::string(m_indentation * 1, '\t');
	}
	std::string lineString(std::string const& _line)
	{
		return indentation() + _line + "\n";
	}
	std::string m_baseType;
	std::ostringstream m_structDef;
	/// Utility type for conveniently composing a tuple
	/// string for struct types.
	StructTupleString m_structTupleString;
	unsigned m_indentation;
	unsigned m_structCounter;
	unsigned m_structStartCounter;
	unsigned m_structFieldCounter;
	bool m_isLastDynParamRightPadded;

	static auto constexpr s_structTypeName = "S";
};

/// Returns a pair of strings, first of which contains assignment statements
/// to initialize a given type, and second of which contains checks to be
/// placed inside the coder function to test abi en/decoding.
class AssignCheckVisitor: public AbiV2ProtoVisitor<std::pair<std::string, std::string>>
{
public:
	AssignCheckVisitor(
		std::string _varName,
		std::string _paramName,
		unsigned _errorStart,
		bool _stateVar,
		unsigned _counter,
		unsigned _structCounter
	)
	{
		m_counter = m_counterStart = _counter;
		m_varName = _varName;
		m_paramName = _paramName;
		m_errorCode = m_errorStart = _errorStart;
		m_indentation = 2;
		m_stateVar = _stateVar;
		m_structCounter = m_structStart = _structCounter;
	}
	std::pair<std::string, std::string> visit(BoolType const&) override;
	std::pair<std::string, std::string> visit(IntegerType const&) override;
	std::pair<std::string, std::string> visit(FixedByteType const&) override;
	std::pair<std::string, std::string> visit(AddressType const&) override;
	std::pair<std::string, std::string> visit(ArrayType const&) override;
	std::pair<std::string, std::string> visit(DynamicByteArrayType const&) override;
	std::pair<std::string, std::string> visit(StructType const&) override;
	using AbiV2ProtoVisitor<std::pair<std::string, std::string>>::visit;

	unsigned errorStmts()
	{
		return m_errorCode - m_errorStart;
	}

	unsigned counted()
	{
		return m_counter - m_counterStart;
	}

	unsigned structs()
	{
		return m_structCounter - m_structStart;
	}

	std::string isabelleValueString()
	{
		return m_valueStream.stream.str();
	}
private:
	struct ValueStream
	{
		ValueStream() = default;
		unsigned index = 0;
		std::ostringstream stream;
		void startStruct()
		{
			if (index >= 1)
				stream << ",";
			index = 0;
			stream << "(";
		}
		void endStruct()
		{
			stream << ")";
		}
		void startArray()
		{
			if (index >= 1)
				stream << ",";
			index = 0;
			stream << "[";
		}
		void endArray()
		{
			stream << "]";
			index++;
		}
		void appendValue(std::string& _value);
	};
	std::string indentation()
	{
		return std::string(m_indentation * 1, '\t');
	}
	unsigned counter()
	{
		return m_counter++;
	}

	std::pair<std::string, std::string> assignAndCheckStringPair(
		std::string const& _varRef,
		std::string const& _checkRef,
		std::string const& _assignValue,
		std::string const& _checkValue,
		DataType _type
	);
	std::string assignString(std::string const&, std::string const&);
	std::string checkString(std::string const&, std::string const&, DataType);
	unsigned m_counter;
	unsigned m_counterStart;
	std::string m_varName;
	std::string m_paramName;
	unsigned m_errorCode;
	unsigned m_errorStart;
	unsigned m_indentation;
	bool m_stateVar;
	unsigned m_structCounter;
	unsigned m_structStart;
	ValueStream m_valueStream;
	bool m_forcedVisit = false;
};

/// Returns a valid value (as a string) for a given type.
class ValueGetterVisitor: AbiV2ProtoVisitor<std::string>
{
public:
	ValueGetterVisitor(unsigned _counter = 0): m_counter(_counter) {}

	std::string visit(BoolType const&) override;
	std::string visit(IntegerType const&) override;
	std::string visit(FixedByteType const&) override;
	std::string visit(AddressType const&) override;
	std::string visit(DynamicByteArrayType const&) override;
	std::string visit(ArrayType const&) override
	{
		solAssert(false, "ABIv2 proto fuzzer: Cannot call valuegettervisitor on complex type");
	}
	std::string visit(StructType const&) override
	{
		solAssert(false, "ABIv2 proto fuzzer: Cannot call valuegettervisitor on complex type");
	}
	using AbiV2ProtoVisitor<std::string>::visit;
	static std::string isabelleAddressValueAsString(std::string& _solAddressString);
	static std::string isabelleBytesValueAsString(std::string& _solFixedBytesString);
private:
	unsigned counter()
	{
		return m_counter++;
	}

	static std::string addressValueAsString(unsigned _counter);
	static std::string fixedByteValueAsString(unsigned _width, unsigned _counter);

	/// Returns a hex literal if _isHexLiteral is true, a string literal otherwise.
	/// The size of the returned literal is _numBytes bytes.
	/// @param _decorate If true, the returned string is enclosed within double quotes
	/// if _isHexLiteral is false.
	/// @param _isHexLiteral If true, the returned string is enclosed within
	/// double quotes prefixed by the string "hex" if _decorate is true. If
	/// _decorate is false, the returned string is returned as-is.
	/// @return hex value as string
	static std::string hexValueAsString(
		unsigned _numBytes,
		unsigned _counter,
		bool _isHexLiteral,
		bool _decorate = true
	);

	/// Returns a hex/string literal of variable length whose value and
	/// size are pseudo-randomly determined from the counter value.
	/// @param _counter A monotonically increasing counter value
	/// @param _isHexLiteral Flag that indicates whether hex (if true) or
	/// string literal (false) is desired
	/// @return A variable length hex/string value
	static std::string bytesArrayValueAsString(unsigned _counter, bool _isHexLiteral);

	/// Concatenates the hash value obtained from monotonically increasing counter
	/// until the desired number of bytes determined by _numBytes.
	/// @param _width Desired number of bytes for hex value
	/// @param _counter A counter value used for creating a keccak256 hash
	/// @param _isHexLiteral Since this routine may be used to construct
	/// string or hex literals, this flag is used to construct a valid output.
	/// @return Valid hex or string literal of size _width bytes
	static std::string variableLengthValueAsString(
		unsigned _width,
		unsigned _counter,
		bool _isHexLiteral
	);

	/// Returns a value that is @a _numBytes bytes long.
	/// @param _numBytes: Number of bytes of desired value
	/// @param _counter: A counter value
	/// @param _isHexLiteral: True if desired value is a hex literal, false otherwise
	static std::string croppedString(unsigned _numBytes, unsigned _counter, bool _isHexLiteral);

	unsigned m_counter;
};

/// Returns true if protobuf array specification is well-formed, false otherwise
class ValidityVisitor: AbiV2ProtoVisitor<bool>
{
public:
	ValidityVisitor(): m_arrayDimensions(0) {}

	bool visit(BoolType const&) override
	{
		return true;
	}

	bool visit(IntegerType const&) override
	{
		return true;
	}

	bool visit(FixedByteType const&) override
	{
		return true;
	}

	bool visit(AddressType const&) override
	{
		return true;
	}

	bool visit(DynamicByteArrayType const&) override
	{
		return true;
	}

	bool visit(ArrayType const& _type) override
	{
		// Mark array type as invalid in one of the following is true
		//  - contains more than s_maxArrayDimensions dimensions
		//  - contains an invalid base type, which happens in the
		//  following cases
		//    - array base type is invalid
		//    - array base type is empty
		m_arrayDimensions++;
		if (m_arrayDimensions > s_maxArrayDimensions)
			return false;
		return visit(_type.t());
	}

	bool visit(StructType const& _type) override
	{
		// A struct is marked invalid only if all of its fields
		// are invalid. This is done to prevent an empty struct
		// being defined (which is a Solidity error).
		for (auto const& t: _type.t())
			if (visit(t))
				return true;
		return false;
	}

	unsigned m_arrayDimensions;
	using AbiV2ProtoVisitor<bool>::visit;
};

/// Returns true if visited type is dynamically encoded by the abi coder,
/// false otherwise.
class DynParamVisitor: AbiV2ProtoVisitor<bool>
{
public:
	DynParamVisitor() = default;

	bool visit(BoolType const&) override
	{
		return false;
	}

	bool visit(IntegerType const&) override
	{
		return false;
	}

	bool visit(FixedByteType const&) override
	{
		return false;
	}

	bool visit(AddressType const&) override
	{
		return false;
	}

	bool visit(DynamicByteArrayType const&) override
	{
		return true;
	}

	bool visit(ArrayType const& _type) override
	{
		// Return early if array spec is not well-formed
		if (!ValidityVisitor().visit(_type))
			return false;

		// Array is dynamically encoded if it at least one of the following is true
		//   - at least one dimension is dynamically sized
		//   - base type is dynamically encoded
		if (!_type.is_static())
			return true;
		else
			return visit(_type.t());
	}

	bool visit(StructType const& _type) override
	{
		// Return early if empty struct
		if (!ValidityVisitor().visit(_type))
			return false;

		// Struct is dynamically encoded if at least one of its fields
		// is dynamically encoded.
		for (auto const& t: _type.t())
			if (visit(t))
				return true;
		return false;
	}

	using AbiV2ProtoVisitor<bool>::visit;
};

}
