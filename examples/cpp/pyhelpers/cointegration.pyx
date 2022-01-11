from libcpp.vector cimport vector
from statsmodels.tsa.vector_ar.vecm import coint_johansen
cimport numpy as np
import numpy as np

cdef public api double py_get_pair_cointegration_ratio(
    vector[double] &price_array_1,
    vector[double] &price_array_2,
):
    cdef double ratio

    price_arrays = np.stack([price_array_1, price_array_2], axis=1)

    try:
        result = coint_johansen(price_arrays, 0, 1)
        
        # The first eigenvector corresponds to the largest eigenvalue
        # i.e., the best candidate for a cointegrating combination
        ratio = min(result.evec[1, 0] / result.evec[0, 0], 0)

    except np.linalg.LinAlgError:
        ratio = 0

    return ratio
