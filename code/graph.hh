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
		Graph(const std::vector<const Variable*> &variables, const std::vector<const Factor*> &factors);
		Graph(const Graph &g);

		std::unordered_set<unsigned> neighbors(unsigned id) const { return _adj.find(id)->second; };
		bool connected(unsigned id1, unsigned id2) const { return _adj.find(id1)->second.count(id2); };

		std::vector<unsigned> ordering(
			const std::vector<const Variable*> &variables,
			unsigned &width,
			std::unordered_map<std::string,bool> &options) const;

		unsigned min_fill(const std::unordered_set<unsigned> &vars) const;
		unsigned weighted_min_fill(const std::unordered_set<unsigned> &vars) const;
		unsigned min_degree(const std::unordered_set<unsigned> &vars) const;

		unsigned order_width(const std::vector<const Variable*> &variables) const;

		friend std::ostream &operator<<(std::ostream &os, const Graph &g);

	private:
		const std::vector<const Variable*> _variables;
		std::unordered_map<unsigned,std::unordered_set<unsigned>> _adj;
	};

	class FactorGraph {
	public:
		FactorGraph(const std::vector<const Variable*> &variables, const std::vector<const Factor*> &factors);
		~FactorGraph();

		unsigned update(unsigned max, double epsilon);
		Factor marginal(const Variable *v) const;

	private:
		std::vector<const Variable*> _variables;
		std::vector<const Factor*> _factors;
		std::unordered_map<unsigned,std::unordered_map<unsigned,const Factor*>> _var2fact_msgs;
		std::unordered_map<unsigned,std::unordered_map<unsigned,const Factor*>> _fact2var_msgs;

		double update_variable_to_factor_msg(unsigned var_id, unsigned factor_id);
		double update_factor_to_variable_msg(unsigned factor_id, unsigned var_id);
	};

}

#endif