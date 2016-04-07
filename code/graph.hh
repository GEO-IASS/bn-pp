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

		std::vector<const Variable*> ordering(const std::vector<const Variable*> &variables) const;

		unsigned min_fill(
			const std::vector<const Variable*> &variables,
			const std::unordered_set<const Variable*> &processed) const;

		friend std::ostream &operator<<(std::ostream &os, const Graph &g);

	private:
		std::unordered_map<const Variable*,std::unordered_set<const Variable*>> _adj;
	};

}

#endif