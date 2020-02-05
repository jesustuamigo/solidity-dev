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

#include <tools/yulPhaser/Population.h>


#include <libsolutil/CommonData.h>
#include <libsolutil/CommonIO.h>

#include <algorithm>
#include <cassert>
#include <numeric>

using namespace std;
using namespace solidity;
using namespace solidity::langutil;
using namespace solidity::util;
using namespace solidity::phaser;

namespace solidity::phaser
{

ostream& operator<<(ostream& _stream, Individual const& _individual);
ostream& operator<<(ostream& _stream, Population const& _population);

}

ostream& phaser::operator<<(ostream& _stream, Individual const& _individual)
{
	_stream << "Fitness: ";
	if (_individual.fitness.has_value())
		_stream << _individual.fitness.value();
	else
		_stream << "<NONE>";
	_stream << ", optimisations: " << _individual.chromosome;

	return _stream;
}

bool phaser::isFitter(Individual const& a, Individual const& b)
{
	assert(a.fitness.has_value() && b.fitness.has_value());

	return (
		(a.fitness.value() < b.fitness.value()) ||
		(a.fitness.value() == b.fitness.value() && a.chromosome.length() < b.chromosome.length()) ||
		(a.fitness.value() == b.fitness.value() && a.chromosome.length() == b.chromosome.length() && toString(a.chromosome) < toString(b.chromosome))
	);
}

Population Population::makeRandom(
	shared_ptr<FitnessMetric const> _fitnessMetric,
	size_t _size,
	function<size_t()> _chromosomeLengthGenerator
)
{
	vector<Individual> individuals;
	for (size_t i = 0; i < _size; ++i)
		individuals.push_back({Chromosome::makeRandom(_chromosomeLengthGenerator())});

	return Population(move(_fitnessMetric), move(individuals));
}

Population Population::makeRandom(
	shared_ptr<FitnessMetric const> _fitnessMetric,
	size_t _size,
	size_t _minChromosomeLength,
	size_t _maxChromosomeLength
)
{
	return makeRandom(
		move(_fitnessMetric),
		_size,
		std::bind(uniformChromosomeLength, _minChromosomeLength, _maxChromosomeLength)
	);
}

void Population::run(optional<size_t> _numRounds, ostream& _outputStream)
{
	doEvaluation();
	for (size_t round = 0; !_numRounds.has_value() || round < _numRounds.value(); ++round)
	{
		doMutation();
		doSelection();
		doEvaluation();

		_outputStream << "---------- ROUND " << round << " ----------" << endl;
		_outputStream << *this;
	}
}

Population operator+(Population _a, Population _b)
{
	// This operator is meant to be used only with populations sharing the same metric (and, to make
	// things simple, "the same" here means the same exact object in memory).
	assert(_a.m_fitnessMetric == _b.m_fitnessMetric);

	return Population(_a.m_fitnessMetric, move(_a.m_individuals) + move(_b.m_individuals));
}

bool Population::operator==(Population const& _other) const
{
	// We consider populations identical only if they share the same exact instance of the metric.
	// It might be possible to define some notion of equality for metric objects but it would
	// be an overkill since mixing populations using different metrics is not a common use case.
	return m_individuals == _other.m_individuals && m_fitnessMetric == _other.m_fitnessMetric;
}

ostream& phaser::operator<<(ostream& _stream, Population const& _population)
{
	auto individual = _population.m_individuals.begin();
	for (; individual != _population.m_individuals.end(); ++individual)
		_stream << *individual << endl;

	return _stream;
}

void Population::doMutation()
{
	// TODO: Implement mutation and crossover
}

void Population::doEvaluation()
{
	for (auto& individual: m_individuals)
		if (!individual.fitness.has_value())
			individual.fitness = m_fitnessMetric->evaluate(individual.chromosome);
}

void Population::doSelection()
{
	m_individuals = sortedIndividuals(move(m_individuals));
	randomizeWorstChromosomes(m_individuals, m_individuals.size() / 2);
}

void Population::randomizeWorstChromosomes(
	vector<Individual>& _individuals,
	size_t _count
)
{
	assert(_individuals.size() >= _count);
	// ASSUMPTION: _individuals is sorted in ascending order

	auto individual = _individuals.begin() + (_individuals.size() - _count);
	for (; individual != _individuals.end(); ++individual)
	{
		*individual = {Chromosome::makeRandom(binomialChromosomeLength(MaxChromosomeLength))};
	}
}

vector<Individual> Population::chromosomesToIndividuals(
	vector<Chromosome> _chromosomes
)
{
	vector<Individual> individuals;
	for (auto& chromosome: _chromosomes)
		individuals.push_back({move(chromosome)});

	return individuals;
}

vector<Individual> Population::sortedIndividuals(vector<Individual> _individuals)
{
	assert(all_of(_individuals.begin(), _individuals.end(), [](auto& i){ return i.fitness.has_value(); }));

	sort(_individuals.begin(), _individuals.end(), isFitter);
	return _individuals;
}
