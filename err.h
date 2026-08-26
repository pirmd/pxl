#ifndef PXL_ERR_H
#define PXL_ERR_H

typedef enum {
    PXL_SUCCESS,
    PXL_E_INVALID_PARAM,
    PXL_E_OUT_OF_MEM,
    PXL_E_BACKEND_INIT,
    PXL_E_BACKEND_FRAME,
} pxl_err_t;

/* Return a static string describing the error code. */
static inline const char *
pxl_err_to_string(pxl_err_t err) {
    switch (err) {
    case PXL_SUCCESS:          return "Success";
    case PXL_E_INVALID_PARAM:  return "Invalid parameter";
    case PXL_E_OUT_OF_MEM:     return "Out of memory";
    case PXL_E_BACKEND_INIT:   return "Backend initialization failed";
    case PXL_E_BACKEND_FRAME:  return "Backend frame failed";
    default:                   return "Unknown error";
    }
}

/* Custom log function type (can be NULL) */
typedef void (*pxl_log_fn)(const char *msg);

/* Set the log callback (NULL = silent, default) */
void pxl_set_log_fn(pxl_log_fn fn);

/* Log a raw message - just a thin wrapper, no formatting */
void pxl_log(const char *msg);


#endif /* PXL_ERR_H */
