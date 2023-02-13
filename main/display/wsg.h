/*! \file wsg.h
 *
 * \section wsg_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section wsg_usage Usage
 *
 * TODO doxygen
 *
 * \section wsg_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef _WSG_H_
#define _WSG_H_

#include <stdint.h>

#include "spiffs_wsg.h"

void drawWsg(const wsg_t* wsg, int16_t xOff, int16_t yOff, bool flipLR, bool flipUD, int16_t rotateDeg);

void drawWsgSimple(const wsg_t* wsg, int16_t xOff, int16_t yOff);

void drawWsgTile(const wsg_t* wsg, int32_t xOff, int32_t yOff);

#endif