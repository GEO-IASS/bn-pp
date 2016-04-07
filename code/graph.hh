#ifndef _BN_GRAPH_H_
#define _BN_GRAPH_H_

#include "variable.hh"
#include "factor.hh"

#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace bn {

	class Graph {
	public:
		Graph(const std::vector<const Factor*> &factors);
		Graph(const Graph &g);

		std::unordered_set<unsigned> neighbors(unsigned id) const { return _adj.find(id)->second; };
		bool connected(unsigned id1, unsigned id2) const { return _adj.find(id1)->second.count(id2); };

		std::vector<unsigned> ordering(const std::vector<const Variable*> &variables, unsigned &width) const;
		unsigned min_fill(const std::unordered_set<unsigned> &vars) const;

		unsigned order_width(const std::vector<const Variable*> &variables) const;

		friend std::ostream &operator<<(std::ostream &os, const Graph &g);

	private:
		std::unordered_map<unsigned,std::unordered_set<unsigned>> _adj;
	};

}

#endif