#include "graph.hh"

#include <iostream>
using namespace std;

namespace bn {

Graph::Graph(const vector<const Factor*> &factors)
{
	for (auto const pf : factors) {
		const Domain &domain = pf->domain();
		unsigned width = domain.width();
		if (width == 0) continue;
		for (unsigned i = 0; i < width-1; ++i) {
			for (unsigned j = i+1; j < width; ++j) {
				const Variable *v1 = domain[i];
				const Variable *v2 = domain[j];
				_adj[v1].insert(v2);
				_adj[v2].insert(v1);
			}
		}
	}
}

vector<const Variable*>
Graph::ordering(const vector<const Variable*> &variables) const
{
	vector<const Variable*> ordering;
	unordered_set<const Variable*> processed;
	while (processed.size() < variables.size()) {
		unsigned min_index = min_fill(variables, processed);
		const Variable *next_var = variables[min_index];
		ordering.push_back(next_var);
		processed.insert(next_var);
	}
	return ordering;
}

unsigned
Graph::min_fill(const vector<const Variable*> &variables, const unordered_set<const Variable*> &processed) const
{
	unsigned min_index = 0;
	unsigned min_fill = _adj.size();

	unsigned nvars = variables.size();
	for (unsigned i = 0; i < nvars; ++i) {

		const Variable *var = variables.at(i);
		if (processed.count(var)) continue;

		unordered_set<const Variable*> neighboors = _adj.find(var)->second;

		unsigned fill_in = 0;
		for (auto const v1 : neighboors) {
			if (processed.count(v1)) continue;
			for (auto const v2 : neighboors) {
				if (processed.count(v2) || v1->id() >= v2->id()) continue;
				if (!_adj.find(v1)->second.count(v2)) {
					fill_in++;
				}
			}
		}

		if (fill_in < min_fill) {
			min_index = i;
			min_fill = fill_in;
		}
	}

	return min_index;
}

ostream &
operator<<(ostream &os, const Graph &g)
{
	os << "Graph:" << endl;
	for (auto it : g._adj) {
		const Variable *v = it.first;
		unordered_set<const Variable*> neighboors = it.second;
		os << v->id() << " :";
		for (auto pv : neighboors) {
			cout << " " << pv->id();
		}
		os << endl;
	}
	os << endl;
	return os;
}

}
