#ifndef _MOBULA_FUNC_
#define _MOBULA_FUNC_

#include "defines.h"
#include "context/context.h"

namespace mobula {


template <typename T, typename UNARY_FUNC>
MOBULA_KERNEL unary_kernel(const int n, const T *a, T *out, UNARY_FUNC func) {
    parfor(n, [&](int i) {
        out[i] = func(a[i]);
    });
}

template <typename T, typename BINARY_FUNC>
MOBULA_KERNEL binary_kernel(const int n, const T *a, const T *b, T *out, BINARY_FUNC func) {
    parfor(n, [&](int i) {
        out[i] = func(a[i], b[i]);
    });
}

/*
 * out[i,j,k,m] += sum(a[i,j,:] * b[k, :, m])
 * for u = 0...n - 1
 *    out[i, k, m] += a[i, u] * b[k, u, m]
 */
template <typename T>
MOBULA_KERNEL dot_add_kernel(const int n, const T *a, const T *b, const int U, const int K, const int M, T *out) {
    parfor(n, [&](int index) {
        const int i = index / (K * U);
        const int k = (index / U) % K;
        const int u = index % U;
        for (int m = 0; m < M; ++m) {
            out[(i * K + k) * M + m] += a[i * U + u] * b[(k * U + u) * M + m];
        }
    });
}

// out[i, j] = sum(a[i, :] * b[:, j])
template <typename T>
MOBULA_KERNEL linalg_gemm_ff_kernel(const int n, const T *a, const T *b, const int U, const int J, T *out) {
    parfor(n, [&](int index) {
        const int i = index / U;
        const int u = index % U;
        for (int j = 0; j < J; ++j) {
            out[i * J + j] += a[i * U + u] * b[u * J + j];
        }
    });
}

// out[i, j] = sum(a[i, :] * b[j, :])
template <typename T>
MOBULA_KERNEL linalg_gemm_ft_kernel(const int n, const T *a, const T *b, const int U, const int J, T *out) {
    parfor(n, [&](int index) {
        const int i = index / J;
        const int j = index % J;
        for (int u = 0; u < U; ++u) {
            out[i * J + j] += a[i * U + u] * b[j * U + u];
        }
    });
}

// out[i, j] = sum(a[:, i] * b[:, j])
template <typename T>
MOBULA_KERNEL linalg_gemm_tf_kernel(const int n, const T *a, const T *b, const int I, const int J, T *out) {
    parfor(n, [&](int index) {
        const int u = index / I;
        const int i = index % I;
        for (int j = 0; j < J; ++j) {
            out[i * J + j] += a[u * I + i] * b[u * J + j];
        }
    });
}

// out[i, j] = sum(a[:, i] * b[j, :])
template <typename T>
MOBULA_KERNEL linalg_gemm_tt_kernel(const int n, const T *a, const T *b, const int I, const int U, const int J, T *out) {
    parfor(n, [&](int index) {
        const int j = index / U;
        const int u = index % U;
        for (int i = 0; i < I; ++i) {
            out[i * J + j] += a[u * I + i] * b[j * U + u];
        }
    });
}

template <typename T>
MOBULA_KERNEL assign_carray_kernel(const int n, const T *a, T *out) {
    parfor(n, [&](int index) {
        out[index] = a[index];
    });
}

template <typename T>
MOBULA_KERNEL assign_val_kernel(const int n, const T val, T *out) {
    parfor(n, [&](int i) {
        out[i] = val;
    });
}

template <typename T>
MOBULA_KERNEL transpose_kernel(const int n, const T *a, const int ndim, const int *shape, const int *strides, T *out) {
    parfor(n, [&](int index) {
        int old_index = 0;
        int r = index;
        for (int i = ndim - 1; i >= 0; --i) {
            int j = r % shape[i];
            r /= shape[i];
            old_index += j * strides[i];
        }
        out[index] = a[old_index];
    });
}


}

extern "C" {
using namespace mobula;

#define REGISTER_UNARY_FUNC(func_name, func) \
    using T = DType;\
    void func_name(const int _n, const T *_a, T *_out) {\
        auto _func = func;\
        KERNEL_RUN((unary_kernel<T, decltype(_func)>), _n)(_n, _a, _out, _func);\
    } \

#define REGISTER_BINARY_FUNC(func_name, func) \
    void func_name(const int _n, const DType *_a, const DType *_b, DType *_out) {\
        auto _func = func;\
        KERNEL_RUN((binary_kernel<DType, decltype(_func)>), _n)(_n, _a, _b, _out, _func);\
    } \

REGISTER_UNARY_FUNC(abs_, []MOBULA_DEVICE(const DType &a){return abs(a);})

REGISTER_BINARY_FUNC(add, []MOBULA_DEVICE(const DType &a, const DType &b){return a + b;})
REGISTER_BINARY_FUNC(sub, []MOBULA_DEVICE(const DType &a, const DType &b){return a - b;})
REGISTER_BINARY_FUNC(mul, []MOBULA_DEVICE(const DType &a, const DType &b){return a * b;})
REGISTER_BINARY_FUNC(div_, []MOBULA_DEVICE(const DType &a, const DType &b){return a / b;})

void dot_add(const DType *a, const DType *b, const int I, const int U, const int K, const int M, DType *out) {
    const int N = I * K * U;
    KERNEL_RUN(dot_add_kernel<DType>, N)(N, a, b, U, K, M, out);
}

void linalg_gemm_ff(const DType *a, const DType *b, const int I, const int U, const int J, DType *out) {
    const int N = I * U;
    KERNEL_RUN(linalg_gemm_ff_kernel<DType>, N)(N, a, b, U, J, out);
}

void linalg_gemm_ft(const DType *a, const DType *b, const int I, const int U, const int J, DType *out) {
    const int N = I * J;
    KERNEL_RUN(linalg_gemm_ft_kernel<DType>, N)(N, a, b, U, J, out);
}

void linalg_gemm_tf(const DType *a, const DType *b, const int I, const int U, const int J, DType *out) {
    const int N = U * I;
    KERNEL_RUN(linalg_gemm_tf_kernel<DType>, N)(N, a, b, I, J, out);
}

void linalg_gemm_tt(const DType *a, const DType *b, const int I, const int U, const int J, DType *out) {
    const int N = J * U;
    KERNEL_RUN(linalg_gemm_tt_kernel<DType>, N)(N, a, b, I, U, J, out);
}

void print_carray(CArray<DType> ca) {
    bool first = true;
    for (size_t i = 0; i < ca.size; ++i) {
        if (!first) std::cout << ", ";
        first = false;
        std::cout << ca.data[i];
    }
    std::cout << std::endl;
}

void assign_carray(CArray<DType> a, DType *out) {
    const int N = a.size;
    auto sp = ctx_pointer<DType>(N, a.data);
    sp.set_ctx(CTX::DEVICE);
    const DType *pa = sp.pointer();
    KERNEL_RUN(assign_carray_kernel<DType>, N)(N, pa, out);
}

void assign_val(const int n, const int val, DType *out) {
    KERNEL_RUN(assign_val_kernel<DType>, n)(n, val, out);
}

void sum(const int n, CArray<DType*> a, DType *out) {
    const int num_vars = a.size;
    assign_val(n, 0, out);
    for (int i = 0; i < num_vars; ++i) {
        DType *e = a.data[i];
        add(n, e, out, out);
    }
}

void transpose(const DType *a, CArray<int> shape, CArray<int> axes, DType *out) {
    ctx_pointer<int> new_shape(shape.size);
    ctx_pointer<int> new_strides(shape.size);
    int *strides = new int[shape.size];
    int n = 1;
    for (size_t i = shape.size; i-- > 0;) {
        strides[i] = n;
        n *= shape[i];
    }
    for (size_t i = 0; i < axes.size; ++i) {
        int j = axes[i];
        new_shape[i] = shape[j];
        new_strides[i] = strides[j];
    }
    new_shape.set_ctx(CTX::DEVICE);
    new_strides.set_ctx(CTX::DEVICE);
    KERNEL_RUN(transpose_kernel<DType>, n)(n, a, shape.size, new_shape.pointer(), new_strides.pointer(), out);
    delete []strides;
}

}

#endif
