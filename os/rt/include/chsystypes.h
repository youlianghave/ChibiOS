/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013,2014 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    chsystypes.h
 * @brief   System types header.
 *
 * @addtogroup scheduler
 * @{
 */

#ifndef _CHSYSTYPES_H_
#define _CHSYSTYPES_H_

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @extends threads_queue_t
 *
 * @brief   Type of a thread structure.
 */
typedef struct ch_thread thread_t;

/**
 * @brief   Type of a generic threads single link list, it works like a stack.
 */
typedef struct ch_threads_list threads_list_t;

/**
 * @extends threads_list_t
 *
 * @brief   Type of a generic threads bidirectional linked list header and element.
 */
typedef struct ch_threads_queue threads_queue_t;

/**
 * @extends threads_queue_t
 *
 * @brief   Type of a ready list header.
 */
typedef struct ch_ready_list ready_list_t;

/**
 * @brief   Type of a Virtual Timer callback function.
 */
typedef void (*vtfunc_t)(void *);

/**
 * @brief   Type of a Virtual Timer structure.
 */
typedef struct ch_virtual_timer virtual_timer_t;

/**
 * @brief   Type of virtual timers list header.
 */
typedef struct ch_virtual_timers_list  virtual_timers_list_t;

/**
 * @brief   Type of system data structure.
 */
typedef struct ch_system ch_system_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* _CHSYSTYPES_H_ */

/** @} */
