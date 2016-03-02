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
    Domain(const Domain &d1, const Domain &d2);

    unsigned width() const { return _width; };
    unsigned size()  const { return _size;  };

    const Variable *operator[](unsigned i) const;

    void next_valuation(std::vector<unsigned> &valuation) const;

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
