from libcpp.vector cimport vector
from statsmodels.tsa.stattools import adfuller
cimport numpy as np
import numpy as np

cdef public api bint py_check_stationary(
    vector[double] &price_array_1,
    vector[double] &price_array_2,
    double ratio,
    double critical_value
):
    lin_comb = np.asarray(price_array_1) - np.asarray(price_array_2) * ratio
    _, p_val, *_ = adfuller(lin_comb)

    return p_val < critical_value
