#ifndef PXL_ERR_H
#define PXL_ERR_H

typedef enum {
    PXL_SUCCESS,
    PXL_E_INVALID_PARAM,
    PXL_E_OUT_OF_MEM,
    PXL_E_BACKEND_INIT,
    PXL_E_BACKEND_FRAME,
} pxl_err_t;

#endif /* PXL_ERR_H */
