//
// Created by Chong Peng on 4/26/16.
//

#include "mpqc/chemistry/qc/lcao/integrals/direct_ao_factory.h"
#include "mpqc/chemistry/qc/lcao/integrals/lcao_factory.h"

namespace mpqc {
namespace lcao {
namespace gaussian {

template class AOFactory<TA::TensorD, TA::SparsePolicy>;

template class DirectAOFactory<TA::TensorD, TA::SparsePolicy>;

}  // namespace gaussian

template class LCAOFactory<TA::TensorD, TA::SparsePolicy>;

}  // namespace lcao
}  // namespace mpqc