/**
 * @file landslide-simics.h
 * @brief simics specific miscellany
 * @author Ben Blum
 */

#ifndef __LS_LANDSLIDE_SIMICS_H
#define __LS_LANDSLIDE_SIMICS_H

#ifndef __LS_LANDSLIDE_H
#error "include landslide.h, do not include this directly"
#endif

#define LANDSLIDE_OBJECT_NAME "landslide0"
#define GET_LANDSLIDE() ((struct ls_state *)SIM_get_object(LANDSLIDE_OBJECT_NAME))

struct ls_state *ls_alloc_init();

#endif
