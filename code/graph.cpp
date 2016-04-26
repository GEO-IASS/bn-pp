#include "graph.hh"

#include <iostream>
using namespace std;

namespace bn {

Graph::Graph(const vector<const Variable*> &variables, const vector<const Factor*> &factors) : _variables(variables)
{
	for (auto const pf : factors) {
		const Domain &domain = pf->domain();
		unsigned width = domain.width();
		for (unsigned i = 0; i < width; ++i) {
			unordered_set<unsigned> empty_set;
			_adj[domain[i]->id()] = empty_set;
		}
	}

	for (auto const pf : factors) {
		const Domain &domain = pf->domain();
		unsigned width = domain.width();

		if (width == 0) continue;

		for (unsigned i = 0; i < width-1; ++i) {
			for (unsigned j = i+1; j < width; ++j) {
				const Variable *v1 = domain[i];
				const Variable *v2 = domain[j];
				_adj[v1->id()].insert(v2->id());
				_adj[v2->id()].insert(v1->id());
			}
		}
	}
}

Graph::Graph(const Graph &g) : _variables(g._variables), _adj(g._adj)
{
}

vector<unsigned>
Graph::ordering(
	const vector<const Variable*> &variables,
	unsigned &width,
	unordered_map<string,bool> &options) const
{

	Graph g(*this);

	unordered_set<unsigned> vars;
	for (auto const pv : variables) {
		vars.insert(pv->id());
	}

	vector<unsigned> ordering;
	width = 0;

	while (!vars.empty()) {

		// find best next_var
		unsigned next_var;
		if (options["min-degree"]) {
			next_var = g.min_degree(vars);
		}
		else if (options["weighted-min-fill"]) {
			next_var = g.weighted_min_fill(vars);
		}
		else {
			next_var = g.min_fill(vars);
		}
		ordering.push_back(next_var);

		// update order width
		unordered_set<unsigned> adj = g.neighbors(next_var);
		unsigned w = adj.size();
		if (w > width) {
			width = w;
		}

		// erase edges to next_var
		for (auto const padj : adj) {
			g._adj[padj].erase(next_var);
		}

		// add fill-in edges
		for (auto const id1 : adj) {
			for (auto const id2 : adj) {
				if (id1 != id2 && !g.connected(id1, id2)) {
					g._adj[id1].insert(id2);
					g._adj[id2].insert(id1);
				}
			}
		}

		// delete next_var
		g._adj.erase(next_var);
		vars.erase(next_var);
	}

	return ordering;
}

unsigned
Graph::min_degree(const unordered_set<unsigned> &vars) const
{
	unsigned next_var = *(vars.begin());
	unsigned min_degree = _adj.size()+1;

	for (auto id : vars) {
		unordered_set<unsigned> adj = neighbors(id);
		unsigned degree = adj.size();

		if (degree < min_degree) {
			next_var = id;
			min_degree = degree;
		}
	}

	return next_var;
}

unsigned
Graph::min_fill(const unordered_set<unsigned> &vars) const
{
	unsigned next_var = *(vars.begin());
	unsigned min_fill = _adj.size()+1;

	for (auto id : vars) {
		unordered_set<unsigned> adj = neighbors(id);

		unsigned fill_in = 0;
		for (auto const id1 : adj) {
			for (auto const id2 : adj) {
				if (id1 < id2 && !connected(id1, id2)) {
					fill_in++;
				}
			}
		}
		if (fill_in < min_fill) {
			next_var = id;
			min_fill = fill_in;
		}
		else if (fill_in == min_fill) { // use min-degree as tiebraker
			unordered_set<unsigned> next_var_adj = neighbors(next_var);
			if (adj.size() < next_var_adj.size()) {
				next_var = id;
				min_fill = fill_in;
			}
		}
	}

	return next_var;
}

unsigned
Graph::weighted_min_fill(const unordered_set<unsigned> &vars) const
{
	unsigned next_var = *(vars.begin());
	unordered_set<unsigned> next_var_adj = neighbors(next_var);

	unsigned weighted_min_fill = 0;
	for (auto const id1 : next_var_adj) {
		for (auto const id2 : next_var_adj) {
			if (id1 < id2 && !connected(id1, id2)) {
				weighted_min_fill += _variables.at(id1)->size() * _variables.at(id2)->size();
			}
		}
	}

	for (auto id : vars) {
		unordered_set<unsigned> adj = neighbors(id);

		unsigned weighted_fill_in = 0;
		for (auto const id1 : adj) {
			for (auto const id2 : adj) {
				if (id1 < id2 && !connected(id1, id2)) {
					weighted_fill_in += _variables.at(id1)->size() * _variables.at(id2)->size();
				}
			}
		}
		if (weighted_fill_in < weighted_min_fill) {
			next_var = id;
			weighted_min_fill = weighted_fill_in;
		}
		else if (weighted_fill_in == weighted_min_fill) { // min-degree
			unordered_set<unsigned> next_var_adj = neighbors(next_var);
			if (adj.size() < next_var_adj.size()) {
				next_var = id;
				weighted_min_fill = weighted_fill_in;
			}
		}
	}

	return next_var;
}

unsigned
Graph::order_width(const vector<const Variable*> &variables) const
{
	Graph g(*this);

	vector<unsigned> vars;
	for (auto const pv : variables) {
		vars.push_back(pv->id());
	}

	unsigned width = 0;
	for (auto next_var : vars) {

		// update order width
		unordered_set<unsigned> adj = g.neighbors(next_var);
		unsigned w = adj.size();
		if (w > width) {
			width = w;
		}

		// delete next_var
		g._adj.erase(next_var);

		// erase edges to next_var
		for (auto const padj : adj) {
			g._adj[padj].erase(next_var);
		}

		// add fill-in edges
		for (auto const id1 : adj) {
			for (auto const id2 : adj) {
				if (id1 != id2 && !g.connected(id1, id2)) {
					g._adj[id1].insert(id2);
					g._adj[id2].insert(id1);
				}
			}
		}
	}

	return width;
}

ostream &
operator<<(ostream &os, const Graph &g)
{
	os << "Graph:" << endl;
	for (auto it : g._adj) {
		unsigned id = it.first;
		unordered_set<unsigned> neighbors = it.second;
		os << id << " :";
		for (auto id2 : neighbors) {
			cout << " " << id2;
		}
		os << endl;
	}
	os << endl;
	return os;
}

FactorGraph::FactorGraph(
	const vector<const Variable*> &variables,
	const vector<const Factor*> &factors)
	: _variables(variables), _factors(factors)
{
	for (unsigned i = 0; i < _factors.size(); ++i) {
		const Domain &d = _factors[i]->domain(); // factor scope

		// initialize messages
		for (unsigned j = 0; j < d.width(); ++j) {
			const Variable *v = d[j];
			unsigned id = v->id();
			unsigned r_j = v->size();
			vector<const Variable*> sc;
			sc.push_back(v);
			_fact2var_msgs[i][id] = new Factor(new Domain(sc), 1.0/r_j);
			_var2fact_msgs[id][i] = new Factor(new Domain(sc), 1.0/r_j);
		}
	}

	// _fact2var_msgs
	// cout << "@ Messages from _fact2var_msgs ..." << endl;
	// for (auto it1 : _fact2var_msgs) {
	// 	cout << ">> factor = ";
	// 	const Factor *f = _factors[it1.first];
	// 	cout << *f;
	// 	cout << ">> scope = " << endl;
	// 	for (auto it2 : it1.second) {
	// 		unsigned id = it2.first;
	// 		const Factor *msg = it2.second;
	// 		cout << ">> var = " << id << endl;
	// 		cout << ">> msg = " << *msg << endl;
	// 	}
	// 	cout << endl;
	// }

	// _var2fact_msgs
	// cout << "@ Messages from _var2fact_msgs ..." << endl;
	// for (auto it1 : _var2fact_msgs) {
	// 	cout << ">> variable = " << it1.first << endl;
	// 	for (auto it2 : it1.second) {
	// 		cout << ">> factor = ";
	// 		const Factor *f = _factors[it2.first];
	// 		cout << *f;
	// 		const Factor *msg = it2.second;
	// 		cout << ">> msg = " << *msg << endl;
	// 	}
	// 	cout << endl;
	// }
}

FactorGraph::~FactorGraph()
{
	// deallocate messages in _fact2var_msgs
	for (auto it1 : _fact2var_msgs) {
		for (auto it2 : it1.second) {
			delete it2.second;
		}
		it1.second.clear();
	}
	_fact2var_msgs.clear();

	// deallocate messages in _var2fact_msgs
	for (auto it1 : _var2fact_msgs) {
		for (auto it2 : it1.second) {
			delete it2.second;
		}
		it1.second.clear();
	}
	_var2fact_msgs.clear();
}

void
FactorGraph::update(unsigned iterations)
{
	for (unsigned i = 0; i < iterations; ++i) {
		// variable to factor
		for (auto it : _var2fact_msgs) {
			unsigned var_id = it.first;
			for (auto it2 : it.second) {
				unsigned fact_id = it2.first;
				update_variable_to_factor_msg(var_id, fact_id);
			}
		}
		// factor to variable
		for (auto it : _fact2var_msgs) {
			unsigned fact_id = it.first;
			for (auto it2 : it.second) {
				unsigned var_id = it2.first;
				update_factor_to_variable_msg(fact_id, var_id);
			}
		}
	}
}

void
FactorGraph::update_variable_to_factor_msg(unsigned var_id, unsigned factor_id)
{
	const Factor *old_msg = _var2fact_msgs[var_id][factor_id];
	delete old_msg;

	Factor new_msg(1.0);
	for (auto it : _var2fact_msgs[var_id]) {
		unsigned f_id = it.first;
		if (f_id == factor_id) continue;
		new_msg *= *(_fact2var_msgs[f_id][var_id]);
	}
	_var2fact_msgs[var_id][factor_id] = new Factor(new_msg.normalize());
}

void
FactorGraph::update_factor_to_variable_msg(unsigned factor_id, unsigned var_id)
{
	const Factor *old_msg = _fact2var_msgs[factor_id][var_id];
	delete old_msg;

	Factor new_msg(*_factors[factor_id]);
	for (auto it : _fact2var_msgs[factor_id]) {
		unsigned v_id = it.first;
		if (v_id == var_id) continue;
		new_msg *= *(_var2fact_msgs[v_id][factor_id]);
		new_msg = new_msg.sum_out(_variables[v_id]);
	}
	_fact2var_msgs[factor_id][var_id] = new Factor(new_msg.normalize());
}

Factor
FactorGraph::marginal(const Variable *v) const
{
	Factor marg(1.0);
	unsigned var_id = v->id();
	for (auto it : _var2fact_msgs.find(var_id)->second) {
		unsigned fact_id = it.first;
		marg *= *(_fact2var_msgs.find(fact_id)->second.find(var_id)->second);
	}
	return marg.normalize();
}

}
