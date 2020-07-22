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


#include <libsmtutil/Sorts.h>

using namespace std;

namespace solidity::smtutil
{

shared_ptr<Sort> const SortProvider::boolSort{make_shared<Sort>(Kind::Bool)};
shared_ptr<IntSort> const SortProvider::uintSort{make_shared<IntSort>(false)};
shared_ptr<IntSort> const SortProvider::sintSort{make_shared<IntSort>(true)};

shared_ptr<IntSort> SortProvider::intSort(bool _signed)
{
	if (_signed)
		return sintSort;
	return uintSort;
}

}
