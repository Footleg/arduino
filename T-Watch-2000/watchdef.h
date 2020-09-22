/******************************************************************************
 * This config file defines which LILYGO product you are compiling for,
 * and includes the LilyGoWatch library.
 * Include this file in all classes using the TTGOClass watch class rather than
 * including the library directly because the library needs the defines before 
 * you include it.
 *****************************************************************************/

#define LILYGO_WATCH_2020_V1              // Using T-Watch2020
#define LILYGO_WATCH_LVGL 

#include <LilyGoWatch.h>
