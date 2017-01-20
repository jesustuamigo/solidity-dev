pragma solidity ^0.4.0;

import "./Token.sol";

contract StandardToken is Token {
	uint256 supply;
	mapping (address => uint256) balance;
	mapping (address =>
		mapping (address => uint256)) m_allowance;

	function StandardToken(address _initialOwner, uint256 _supply) {
		supply = _supply;
		balance[_initialOwner] = _supply;
	}

	function balanceOf(address _account) constant returns (uint) {
		return balance[_account];
	}

	function totalSupply() constant returns (uint) {
		return supply;
	}

	function transfer(address _to, uint256 _value) returns (bool success) {
		if (balance[msg.sender] >= _value && balance[_to] + _value >= balance[_to]) {
			balance[msg.sender] -= _value;
			balance[_to] += _value;
			Transfer(msg.sender, _to, _value);
			return true;
		} else {
			return false;
		}
	}

	function transferFrom(address _from, address _to, uint256 _value) returns (bool success) {
		if (m_allowance[_from][msg.sender] >= _value && balance[_to] + _value >= balance[_to]) {
			m_allowance[_from][msg.sender] -= _value;
			balance[_to] += _value;
			Transfer(_from, _to, _value);
			return true;
		} else {
			return false;
		}
	}

	function approve(address _spender, uint256 _value) returns (bool success) {
		m_allowance[msg.sender][_spender] = _value;
		Approval(msg.sender, _spender, _value);
		return true;
	}

	function allowance(address _owner, address _spender) constant returns (uint256 remaining) {
		return m_allowance[_owner][_spender];
	}
}
