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
				_adj[v1->id()].insert(v2->id());
				_adj[v2->id()].insert(v1->id());
			}
		}
	}
}

Graph::Graph(const Graph &g) : _adj(g._adj)
{

}

vector<unsigned>
Graph::ordering(const vector<const Variable*> &variables, unsigned &width) const
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
		unsigned next_var = g.min_fill(vars);
		ordering.push_back(next_var);

		// update order width
		unordered_set<unsigned> adj = g.neighbors(next_var);
		unsigned w = adj.size();
		if (w > width) {
			width = w;
		}

		// delete next_var
		g._adj.erase(next_var);
		vars.erase(next_var);

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

	return ordering;
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
		else if (fill_in == min_fill) { // min-degree
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

}
