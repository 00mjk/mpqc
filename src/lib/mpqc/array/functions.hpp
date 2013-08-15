#ifndef MPQC_ARRAY_FUNCTIONS_HPP
#define MPQC_ARRAY_FUNCTIONS_HPP

#include "mpqc/array.hpp"
#include "mpqc/math/matrix.hpp"

namespace mpqc {

    template<typename T, typename U>
    void symmetrize(Array<T> &A, U &counter, size_t B = 1024) {
        assert(A.rank() == 2);
        assert(A.dims()[0] == A.dims()[1]);

        size_t N = A.dims()[0];
        matrix<T> Aij(B,B), Aji(B,B);

        for (size_t j = 0, ij = 0, next = counter++; j < N; j += B) {
            for (size_t i = 0; i <= j; i += B, ++ij) {

                if (ij != next) continue;
                next = counter++;

                range ri(i, std::min(i+B,N));
                range rj(j, std::min(j+B,N));

                Aij.resize(ri.size(), rj.size());
                Aji.resize(rj.size(), ri.size());
                A(ri,rj) >> Aij;
                A(rj,ri) >> Aji;
                Aij = Aij + Aji.transpose();
                Aji = Aij.transpose();
                A(ri,rj) << Aij;
                A(rj,ri) << Aji;                
            }
        }
    }

    template<typename T>
    void symmetrize(Array<T> &A, size_t block = 1024) {
        size_t counter = 0;
        symmetrize(A, counter);
    }

} // namespace mpqc

#endif /* MPQC_ARRAY_FUNCTIONS_HPP */
