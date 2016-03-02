#ifndef _BN_IO_FILE_H_
#define _BN_IO_FILE_H_

#include "model.hh"

namespace bn {

int
read_uai_model(const char *filename, unsigned &order, BN **bn);

}

#endif
