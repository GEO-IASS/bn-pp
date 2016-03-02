#ifndef _BN_VARIABLE_H_
#define _BN_VARIABLE_H_

#include <ostream>

namespace bn {

class Variable {
public:
    Variable(unsigned id, unsigned size);

    unsigned id()   const { return _id;   }
    unsigned size() const { return _size; }

    friend std::ostream &operator<<(std::ostream &o, const Variable &v);

private:
    unsigned _id;
    unsigned _size;
};

}

#endif