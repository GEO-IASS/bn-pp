#ifndef _BN_DOMAIN_H_
#define _BN_DOMAIN_H_

#include "variable.hh"

#include <vector>
#include <unordered_map>

namespace bn {

class Domain {
public:
    Domain();
    Domain(std::vector<const Variable*> scope);
    Domain(const Domain &d);
    Domain(const Domain &d1, const Domain &d2);
    Domain(const Domain &d, const Variable *v);
    Domain(const Domain &d, const std::unordered_map<unsigned,unsigned> &evidence);

    std::vector<const Variable*> scope() const { return _scope; };
    unsigned width() const { return _width; };
    unsigned size()  const { return _size;  };

    const Variable *operator[](unsigned i) const;

    bool in_scope(const Variable* v) const;
    bool in_scope(unsigned id) const;

    void next_valuation(std::vector<unsigned> &valuation) const;
    void next_valuation_with_evidence(std::vector<unsigned> &valuation, const std::unordered_map<unsigned,unsigned> &evidence) const;
    void update_valuation_with_evidence(std::vector<unsigned> &valuation, const std::unordered_map<unsigned,unsigned> &evidence) const;

    unsigned position_valuation(std::vector<unsigned> valuation) const;
    unsigned position_consistent_valuation(std::vector<unsigned> valuation, const Domain &domain) const;
    unsigned position_consistent_valuation(std::vector<unsigned> valuation, const Domain &domain, const Variable *v, unsigned value) const;

    friend std::ostream &operator<<(std::ostream &o, const Domain &v);

private:
    std::vector<const Variable*> _scope;
    unsigned _width;
    unsigned _size;
    std::vector<unsigned> _offset;
    std::unordered_map<unsigned, unsigned> _var_to_index;
};

}

#endif
