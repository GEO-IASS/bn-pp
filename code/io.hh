#ifndef _BN_IO_FILE_H_
#define _BN_IO_FILE_H_

#include <vector>

#include "variable.hh"
#include "factor.hh"

namespace bn {

int
read_uai_model(const char *filename, unsigned &order, std::vector<Variable*> &variables, std::vector<Factor*> &factors);

}

#endif
