//
// Created by josh on 28/10/2020.
//

#ifndef SPE_ARRAY2D_H
#define SPE_ARRAY2D_H
#include <array>
namespace sel {
    template<class T, size_t Rows,  size_t Cols>struct array2d : std::array<T,  Rows * Cols> {
        T& operator()(size_t row, size_t col) { return (*this)[row * Cols + col]; }
    };
}
#endif //SPE_ARRAY2D_H
