#ifndef PXL_H
#define PXL_H

/*
 * PXL umbrella header - includes all public API headers.
 * For finer granularity, include individual headers directly.
 */

/* Core types and error handling */
#include "err.h"
#include "geom.h"
#include "color.h"
#include "buf.h"

/* Drawing */
#include "canvas.h"
#include "camera.h"
#include "shape.h"
#include "blit.h"
#include "text_basic.h"

/* Text */
#include "text.h"

/* Tileset */
#include "tileset.h"

/* Input and time */
#include "input.h"
#include "stepper.h"

/* Backend (platform-specific) */
#include "backend.h"

/* App layer */
#include "app.h"

#endif /* PXL_H */
