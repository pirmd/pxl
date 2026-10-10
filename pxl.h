#ifndef PXL_H
#define PXL_H

/*
 * PXL umbrella header - includes all public API headers.
 * For finer granularity, include individual headers directly.
 */

/* Core types and error handling */
#include "buf.h"
#include "err.h"
#include "geom.h"
#include "backend.h"
#include "input.h"

/* Drawing */
#include "color.h"
#include "canvas.h"
#include "shape.h"
#include "blit.h"
#include "bitmask.h"
#include "text_basic.h"

/* Text */
#include "text.h"

/* Framework */
#include "app.h"
#include "camera.h"
#include "layout.h"
#include "stepper.h"
#include "tileset.h"
#include "timer.h"


#endif /* PXL_H */
