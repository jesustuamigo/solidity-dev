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
/** @file StringUtils.h
 * @author Balajiganapathi S <balajiganapathi.s@gmail.com>
 * @date 2017
 *
 * String routines
 */

#include "StringUtils.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace std;
using namespace dev;

bool dev::stringWithinDistance(string const& _str1, string const& _str2, size_t _maxDistance)
{
	if (_str1 == _str2)
		return true;

	size_t n1 = _str1.size();
	size_t n2 = _str2.size();
	size_t distance = stringDistance(_str1, _str2);

	// if distance is not greater than _maxDistance, and distance is strictly less than length of both names, they can be considered similar
	// this is to avoid irrelevant suggestions
	return distance <= _maxDistance && distance < n1 && distance < n2;
}

size_t dev::stringDistance(string const& _str1, string const& _str2)
{
	size_t n1 = _str1.size();
	size_t n2 = _str2.size();
	// Optimize by storing only last 2 rows and current row. So first index is considered modulo 3
	vector<vector<size_t>> dp(3, vector<size_t>(n2 + 1));

	// In this dp formulation of Damerau–Levenshtein distance we are assuming that the strings are 1-based to make base case storage easier.
	// So index accesser to _name1 and _name2 have to be adjusted accordingly
	for (size_t i1 = 0; i1 <= n1; ++i1)
	{
		for (size_t i2 = 0; i2 <= n2; ++i2)
		{
				if (min(i1, i2) == 0) // base case
					dp[i1 % 3][i2] = max(i1, i2);
				else
				{
					dp[i1 % 3][i2] = min(dp[(i1-1) % 3][i2] + 1, dp[i1 % 3][i2-1] + 1); // deletion and insertion
					if (_str1[i1-1] == _str2[i2-1])  // same chars, can skip
						dp[i1 % 3][i2] = min(dp[i1 % 3][i2], dp[(i1-1) % 3][i2-1]);
					else // different chars so try substitution
						dp[i1 % 3][i2] = min(dp[i1 % 3][i2], dp[(i1-1) % 3][i2-1] + 1);

					if (i1 > 1 && i2 > 1 && _str1[i1-1] == _str2[i2-2] && _str1[i1-2] == _str2[i2-1]) // Try transposing
						dp[i1 % 3][i2] = min(dp[i1 % 3][i2], dp[(i1-2) % 3][i2-2] + 1);
				}
		}
	}

	return dp[n1 % 3][n2];
}

string dev::quotedAlternativesList(vector<string> const& suggestions)
{
	if (suggestions.empty())
		return "";
	if (suggestions.size() == 1)
		return "\"" + suggestions.front() + "\"";

	string choices = "\"" + suggestions.front() + "\"";
	for (size_t i = 1; i + 1 < suggestions.size(); ++i)
		choices += ", \"" + suggestions[i] + "\"";

	choices += " or \"" + suggestions.back() + "\"";

	return choices;
}

