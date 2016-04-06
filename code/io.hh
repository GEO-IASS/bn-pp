#ifndef _BN_IO_FILE_H_
#define _BN_IO_FILE_H_

#include <string>
#include <unordered_map>

#include "model.hh"

namespace bn {

int
read_uai_model(std::string &filename, BN **model);

int
read_uai_model(std::string &filename, MN **model);

int
read_uai_evidence(std::string &filename, std::unordered_map<unsigned,unsigned> &evidence);

}

#endif
