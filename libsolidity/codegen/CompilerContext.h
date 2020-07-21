/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Utilities for the solidity compiler.
 */

#pragma once

#include <libsolidity/ast/ASTAnnotations.h>
#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/ast/Types.h>
#include <libsolidity/codegen/ABIFunctions.h>

#include <libsolidity/interface/DebugSettings.h>
#include <libsolidity/interface/OptimiserSettings.h>

#include <libevmasm/Assembly.h>
#include <libevmasm/Instruction.h>
#include <liblangutil/ErrorReporter.h>
#include <liblangutil/EVMVersion.h>
#include <libsolutil/Common.h>

#include <libyul/AsmAnalysisInfo.h>
#include <libyul/backends/evm/EVMDialect.h>

#include <functional>
#include <ostream>
#include <stack>
#include <queue>
#include <utility>

namespace solidity::frontend {

class Compiler;

/**
 * Context to be shared by all units that compile the same contract.
 * It stores the generated bytecode and the position of identifiers in memory and on the stack.
 */
class CompilerContext
{
public:
	explicit CompilerContext(
		langutil::EVMVersion _evmVersion,
		RevertStrings _revertStrings,
		CompilerContext* _runtimeContext = nullptr
	):
		m_asm(std::make_shared<evmasm::Assembly>()),
		m_evmVersion(_evmVersion),
		m_revertStrings(_revertStrings),
		m_reservedMemory{0},
		m_runtimeContext(_runtimeContext),
		m_abiFunctions(m_evmVersion, m_revertStrings, m_yulFunctionCollector),
		m_yulUtilFunctions(m_evmVersion, m_revertStrings, m_yulFunctionCollector)
	{
		if (m_runtimeContext)
			m_runtimeSub = size_t(m_asm->newSub(m_runtimeContext->m_asm).data());
	}

	langutil::EVMVersion const& evmVersion() const { return m_evmVersion; }

	/// Update currently enabled set of experimental features.
	void setExperimentalFeatures(std::set<ExperimentalFeature> const& _features) { m_experimentalFeatures = _features; }
	/// @returns true if the given feature is enabled.
	bool experimentalFeatureActive(ExperimentalFeature _feature) const { return m_experimentalFeatures.count(_feature); }

	void addStateVariable(VariableDeclaration const& _declaration, u256 const& _storageOffset, unsigned _byteOffset);
	void addImmutable(VariableDeclaration const& _declaration);

	/// @returns the reserved memory for storing the value of the immutable @a _variable during contract creation.
	size_t immutableMemoryOffset(VariableDeclaration const& _variable) const;
	/// @returns a list of slot names referring to the stack slots of an immutable variable.
	static std::vector<std::string> immutableVariableSlotNames(VariableDeclaration const& _variable);

	/// @returns the reserved memory and resets it to mark it as used.
	size_t reservedMemory();

	void addVariable(VariableDeclaration const& _declaration, unsigned _offsetToCurrent = 0);
	void removeVariable(Declaration const& _declaration);
	/// Removes all local variables currently allocated above _stackHeight.
	void removeVariablesAboveStackHeight(unsigned _stackHeight);
	/// Returns the number of currently allocated local variables.
	unsigned numberOfLocalVariables() const;

	void setOtherCompilers(std::map<ContractDefinition const*, std::shared_ptr<Compiler const>> const& _otherCompilers) { m_otherCompilers = _otherCompilers; }
	std::shared_ptr<evmasm::Assembly> compiledContract(ContractDefinition const& _contract) const;
	std::shared_ptr<evmasm::Assembly> compiledContractRuntime(ContractDefinition const& _contract) const;

	void setStackOffset(int _offset) { m_asm->setDeposit(_offset); }
	void adjustStackOffset(int _adjustment) { m_asm->adjustDeposit(_adjustment); }
	unsigned stackHeight() const { solAssert(m_asm->deposit() >= 0, ""); return unsigned(m_asm->deposit()); }

	bool isLocalVariable(Declaration const* _declaration) const;
	bool isStateVariable(Declaration const* _declaration) const { return m_stateVariables.count(_declaration) != 0; }

	/// @returns the entry label of the given function and creates it if it does not exist yet.
	evmasm::AssemblyItem functionEntryLabel(Declaration const& _declaration);
	/// @returns the entry label of the given function. Might return an AssemblyItem of type
	/// UndefinedItem if it does not exist yet.
	evmasm::AssemblyItem functionEntryLabelIfExists(Declaration const& _declaration) const;
	/// @returns the function that overrides the given declaration from the most derived class just
	/// above _base in the current inheritance hierarchy.
	FunctionDefinition const& superFunction(FunctionDefinition const& _function, ContractDefinition const& _base);
	/// Sets the contract currently being compiled - the most derived one.
	void setMostDerivedContract(ContractDefinition const& _contract) { m_mostDerivedContract = &_contract; }
	ContractDefinition const& mostDerivedContract() const;

	/// @returns the next function in the queue of functions that are still to be compiled
	/// (i.e. that were referenced during compilation but where we did not yet generate code for).
	/// Returns nullptr if the queue is empty. Does not remove the function from the queue,
	/// that will only be done by startFunction below.
	Declaration const* nextFunctionToCompile() const;
	/// Resets function specific members, inserts the function entry label and marks the function
	/// as "having code".
	void startFunction(Declaration const& _function);

	/// Appends a call to the named low-level function and inserts the generator into the
	/// list of low-level-functions to be generated, unless it already exists.
	/// Note that the generator should not assume that objects are still alive when it is called,
	/// unless they are guaranteed to be alive for the whole run of the compiler (AST nodes, for example).
	void callLowLevelFunction(
		std::string const& _name,
		unsigned _inArgs,
		unsigned _outArgs,
		std::function<void(CompilerContext&)> const& _generator
	);

	/// Appends a call to a yul function and registers the function as externally used.
	void callYulFunction(
		std::string const& _name,
		unsigned _inArgs,
		unsigned _outArgs
	);

	/// Returns the tag of the named low-level function and inserts the generator into the
	/// list of low-level-functions to be generated, unless it already exists.
	/// Note that the generator should not assume that objects are still alive when it is called,
	/// unless they are guaranteed to be alive for the whole run of the compiler (AST nodes, for example).
	evmasm::AssemblyItem lowLevelFunctionTag(
		std::string const& _name,
		unsigned _inArgs,
		unsigned _outArgs,
		std::function<void(CompilerContext&)> const& _generator
	);
	/// Generates the code for missing low-level functions, i.e. calls the generators passed above.
	void appendMissingLowLevelFunctions();
	ABIFunctions& abiFunctions() { return m_abiFunctions; }
	YulUtilFunctions& utilFunctions() { return m_yulUtilFunctions; }
	/// @returns concatenation of all generated functions and a set of the
	/// externally used functions.
	/// Clears the internal list, i.e. calling it again will result in an
	/// empty return value.
	std::pair<std::string, std::set<std::string>> requestedYulFunctions();
	bool requestedYulFunctionsRan() const { return m_requestedYulFunctionsRan; }

	/// Returns the distance of the given local variable from the bottom of the stack (of the current function).
	unsigned baseStackOffsetOfVariable(Declaration const& _declaration) const;
	/// If supplied by a value returned by @ref baseStackOffsetOfVariable(variable), returns
	/// the distance of that variable from the current top of the stack.
	unsigned baseToCurrentStackOffset(unsigned _baseOffset) const;
	/// Converts an offset relative to the current stack height to a value that can be used later
	/// with baseToCurrentStackOffset to point to the same stack element.
	unsigned currentToBaseStackOffset(unsigned _offset) const;
	/// @returns pair of slot and byte offset of the value inside this slot.
	std::pair<u256, unsigned> storageLocationOfVariable(Declaration const& _declaration) const;

	/// Appends a JUMPI instruction to a new tag and @returns the tag
	evmasm::AssemblyItem appendConditionalJump() { return m_asm->appendJumpI().tag(); }
	/// Appends a JUMPI instruction to @a _tag
	CompilerContext& appendConditionalJumpTo(evmasm::AssemblyItem const& _tag) { m_asm->appendJumpI(_tag); return *this; }
	/// Appends a JUMP to a new tag and @returns the tag
	evmasm::AssemblyItem appendJumpToNew() { return m_asm->appendJump().tag(); }
	/// Appends a JUMP to a tag already on the stack
	CompilerContext& appendJump(evmasm::AssemblyItem::JumpType _jumpType = evmasm::AssemblyItem::JumpType::Ordinary);
	/// Appends an INVALID instruction
	CompilerContext& appendInvalid();
	/// Appends a conditional INVALID instruction
	CompilerContext& appendConditionalInvalid();
	/// Appends a REVERT(0, 0) call
	/// @param _message is an optional revert message used in debug mode
	CompilerContext& appendRevert(std::string const& _message = "");
	/// Appends a conditional REVERT-call, either forwarding the RETURNDATA or providing the
	/// empty string. Consumes the condition.
	/// If the current EVM version does not support RETURNDATA, uses REVERT but does not forward
	/// the data.
	/// @param _message is an optional revert message used in debug mode
	CompilerContext& appendConditionalRevert(bool _forwardReturnData = false, std::string const& _message = "");
	/// Appends a JUMP to a specific tag
	CompilerContext& appendJumpTo(
		evmasm::AssemblyItem const& _tag,
		evmasm::AssemblyItem::JumpType _jumpType = evmasm::AssemblyItem::JumpType::Ordinary
	) { *m_asm << _tag.pushTag(); return appendJump(_jumpType); }
	/// Appends pushing of a new tag and @returns the new tag.
	evmasm::AssemblyItem pushNewTag() { return m_asm->append(m_asm->newPushTag()).tag(); }
	/// @returns a new tag without pushing any opcodes or data
	evmasm::AssemblyItem newTag() { return m_asm->newTag(); }
	/// @returns a new tag identified by name.
	evmasm::AssemblyItem namedTag(std::string const& _name) { return m_asm->namedTag(_name); }
	/// Adds a subroutine to the code (in the data section) and pushes its size (via a tag)
	/// on the stack. @returns the pushsub assembly item.
	evmasm::AssemblyItem addSubroutine(evmasm::AssemblyPointer const& _assembly) { return m_asm->appendSubroutine(_assembly); }
	/// Pushes the size of the subroutine.
	void pushSubroutineSize(size_t _subRoutine) { m_asm->pushSubroutineSize(_subRoutine); }
	/// Pushes the offset of the subroutine.
	void pushSubroutineOffset(size_t _subRoutine) { m_asm->pushSubroutineOffset(_subRoutine); }
	/// Pushes the size of the final program
	void appendProgramSize() { m_asm->appendProgramSize(); }
	/// Adds data to the data section, pushes a reference to the stack
	evmasm::AssemblyItem appendData(bytes const& _data) { return m_asm->append(_data); }
	/// Appends the address (virtual, will be filled in by linker) of a library.
	void appendLibraryAddress(std::string const& _identifier) { m_asm->appendLibraryAddress(_identifier); }
	/// Appends an immutable variable. The value will be filled in by the constructor.
	void appendImmutable(std::string const& _identifier) { m_asm->appendImmutable(_identifier); }
	/// Appends an assignment to an immutable variable. Only valid in creation code.
	void appendImmutableAssignment(std::string const& _identifier) { m_asm->appendImmutableAssignment(_identifier); }
	/// Appends a zero-address that can be replaced by something else at deploy time (if the
	/// position in bytecode is known).
	void appendDeployTimeAddress() { m_asm->append(evmasm::PushDeployTimeAddress); }
	/// Resets the stack of visited nodes with a new stack having only @c _node
	void resetVisitedNodes(ASTNode const* _node);
	/// Pops the stack of visited nodes
	void popVisitedNodes() { m_visitedNodes.pop(); updateSourceLocation(); }
	/// Pushes an ASTNode to the stack of visited nodes
	void pushVisitedNodes(ASTNode const* _node) { m_visitedNodes.push(_node); updateSourceLocation(); }

	/// Append elements to the current instruction list and adjust @a m_stackOffset.
	CompilerContext& operator<<(evmasm::AssemblyItem const& _item) { m_asm->append(_item); return *this; }
	CompilerContext& operator<<(evmasm::Instruction _instruction) { m_asm->append(_instruction); return *this; }
	CompilerContext& operator<<(u256 const& _value) { m_asm->append(_value); return *this; }
	CompilerContext& operator<<(bytes const& _data) { m_asm->append(_data); return *this; }

	/// Appends inline assembly (strict mode).
	/// @a _replacements are string-matching replacements that are performed prior to parsing the inline assembly.
	/// @param _localVariables assigns stack positions to variables with the last one being the stack top
	/// @param _externallyUsedFunctions a set of function names that are not to be renamed or removed.
	/// @param _system if true, this is a "system-level" assembly where all functions use named labels.
	void appendInlineAssembly(
		std::string const& _assembly,
		std::vector<std::string> const& _localVariables = std::vector<std::string>(),
		std::set<std::string> const& _externallyUsedFunctions = std::set<std::string>(),
		bool _system = false,
		OptimiserSettings const& _optimiserSettings = OptimiserSettings::none()
	);

	/// If m_revertStrings is debug, @returns inline assembly code that
	/// stores @param _message in memory position 0 and reverts.
	/// Otherwise returns "revert(0, 0)".
	std::string revertReasonIfDebug(std::string const& _message = "");

	void optimizeYul(yul::Object& _object, yul::EVMDialect const& _dialect, OptimiserSettings const& _optimiserSetting, std::set<yul::YulString> const& _externalIdentifiers = {});

	/// Appends arbitrary data to the end of the bytecode.
	void appendAuxiliaryData(bytes const& _data) { m_asm->appendAuxiliaryDataToEnd(_data); }

	/// Run optimisation step.
	void optimise(OptimiserSettings const& _settings) { m_asm->optimise(translateOptimiserSettings(_settings)); }

	/// @returns the runtime context if in creation mode and runtime context is set, nullptr otherwise.
	CompilerContext* runtimeContext() const { return m_runtimeContext; }
	/// @returns the identifier of the runtime subroutine.
	size_t runtimeSub() const { return m_runtimeSub; }

	/// @returns a const reference to the underlying assembly.
	evmasm::Assembly const& assembly() const { return *m_asm; }
	/// @returns a shared pointer to the assembly.
	/// Should be avoided except when adding sub-assemblies.
	std::shared_ptr<evmasm::Assembly> assemblyPtr() const { return m_asm; }

	/// @arg _sourceCodes is the map of input files to source code strings
	std::string assemblyString(StringMap const& _sourceCodes = StringMap()) const
	{
		return m_asm->assemblyString(_sourceCodes);
	}

	/// @arg _sourceCodes is the map of input files to source code strings
	Json::Value assemblyJSON(std::map<std::string, unsigned> const& _indicies = std::map<std::string, unsigned>()) const
	{
		return m_asm->assemblyJSON(_indicies);
	}

	evmasm::LinkerObject const& assembledObject() const;
	evmasm::LinkerObject const& assembledRuntimeObject(size_t _subIndex) const { return m_asm->sub(_subIndex).assemble(); }

	/**
	 * Helper class to pop the visited nodes stack when a scope closes
	 */
	class LocationSetter: public ScopeGuard
	{
	public:
		LocationSetter(CompilerContext& _compilerContext, ASTNode const& _node):
			ScopeGuard([&]{ _compilerContext.popVisitedNodes(); }) { _compilerContext.pushVisitedNodes(&_node); }
	};

	void setModifierDepth(size_t _modifierDepth) { m_asm->m_currentModifierDepth = _modifierDepth; }

	RevertStrings revertStrings() const { return m_revertStrings; }

private:
	/// Updates source location set in the assembly.
	void updateSourceLocation();

	evmasm::Assembly::OptimiserSettings translateOptimiserSettings(OptimiserSettings const& _settings);

	/**
	 * Helper class that manages function labels and ensures that referenced functions are
	 * compiled in a specific order.
	 */
	struct FunctionCompilationQueue
	{
		/// @returns the entry label of the given function and creates it if it does not exist yet.
		/// @param _context compiler context used to create a new tag if needed
		evmasm::AssemblyItem entryLabel(Declaration const& _declaration, CompilerContext& _context);
		/// @returns the entry label of the given function. Might return an AssemblyItem of type
		/// UndefinedItem if it does not exist yet.
		evmasm::AssemblyItem entryLabelIfExists(Declaration const& _declaration) const;

		/// @returns the next function in the queue of functions that are still to be compiled
		/// (i.e. that were referenced during compilation but where we did not yet generate code for).
		/// Returns nullptr if the queue is empty. Does not remove the function from the queue,
		/// that will only be done by startFunction below.
		Declaration const* nextFunctionToCompile() const;
		/// Informs the queue that we are about to compile the given function, i.e. removes
		/// the function from the queue of functions to compile.
		void startFunction(Declaration const& _function);

		/// Labels pointing to the entry points of functions.
		std::map<Declaration const*, evmasm::AssemblyItem> m_entryLabels;
		/// Set of functions for which we did not yet generate code.
		std::set<Declaration const*> m_alreadyCompiledFunctions;
		/// Queue of functions that still need to be compiled (important to be a queue to maintain
		/// determinism even in the presence of a non-deterministic allocator).
		/// Mutable because we will throw out some functions earlier than needed.
		mutable std::queue<Declaration const*> m_functionsToCompile;
	} m_functionCompilationQueue;

	evmasm::AssemblyPointer m_asm;
	/// Version of the EVM to compile against.
	langutil::EVMVersion m_evmVersion;
	RevertStrings const m_revertStrings;
	/// Activated experimental features.
	std::set<ExperimentalFeature> m_experimentalFeatures;
	/// Other already compiled contracts to be used in contract creation calls.
	std::map<ContractDefinition const*, std::shared_ptr<Compiler const>> m_otherCompilers;
	/// Storage offsets of state variables
	std::map<Declaration const*, std::pair<u256, unsigned>> m_stateVariables;
	/// Memory offsets reserved for the values of immutable variables during contract creation.
	std::map<VariableDeclaration const*, size_t> m_immutableVariables;
	/// Total amount of reserved memory. Reserved memory is used to store immutable variables during contract creation.
	/// This has to be finalized before initialiseFreeMemoryPointer() is called. That function
	/// will reset the optional to verify that.
	std::optional<size_t> m_reservedMemory = {0};
	/// Offsets of local variables on the stack (relative to stack base).
	/// This needs to be a stack because if a modifier contains a local variable and this
	/// modifier is applied twice, the position of the variable needs to be restored
	/// after the nested modifier is left.
	std::map<Declaration const*, std::vector<unsigned>> m_localVariables;
	/// The contract currently being compiled. Virtual function lookup starts from this contarct.
	ContractDefinition const* m_mostDerivedContract = nullptr;
	/// Stack of current visited AST nodes, used for location attachment
	std::stack<ASTNode const*> m_visitedNodes;
	/// The runtime context if in Creation mode, this is used for generating tags that would be stored into the storage and then used at runtime.
	CompilerContext *m_runtimeContext;
	/// The index of the runtime subroutine.
	size_t m_runtimeSub = std::numeric_limits<size_t>::max();
	/// An index of low-level function labels by name.
	std::map<std::string, evmasm::AssemblyItem> m_lowLevelFunctions;
	/// Collector for yul functions.
	MultiUseYulFunctionCollector m_yulFunctionCollector;
	/// Set of externally used yul functions.
	std::set<std::string> m_externallyUsedYulFunctions;
	/// Container for ABI functions to be generated.
	ABIFunctions m_abiFunctions;
	/// Container for Yul Util functions to be generated.
	YulUtilFunctions m_yulUtilFunctions;
	/// The queue of low-level functions to generate.
	std::queue<std::tuple<std::string, unsigned, unsigned, std::function<void(CompilerContext&)>>> m_lowLevelFunctionGenerationQueue;
	/// Flag to check that requestedYulFunctions() was called exactly once
	bool m_requestedYulFunctionsRan = false;
};

}
