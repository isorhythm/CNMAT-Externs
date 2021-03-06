/** @file cmmjl_errno.h
	Error codes.
	@authors John MacCallum, The Center for New Music and Audio Technologies, University of California, Berkeley.
	@addtogroup Error 
@{
*/
/*
Copyright (c) 2008.  The Regents of the University of California (Regents).
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by John MacCallum, The Center for New Music and 
Audio Technologies, University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
     ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
     REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.

*/

#ifndef __CMMJL_ERRNO_H__
#define __CMMJL_ERRNO_H__

/** Error codes are 64-bit unsigned ints */
typedef unsigned long long t_cmmjl_error;

/** @name Error codes */
/*@{*/
/* general errors */
#define	CMMJL_SUCCESS 0ull /**< success */
#define	CMMJL_FAILURE 1ull /**< failure */
#define	CMMJL_ENULLPTR 2ull /**< null pointer */
#define	CMMJL_EBADTYPE 3ull /**< unknown data type */
#define CMMJL_ENODATA 4ull /**< no data */
#define CMMJL_ENOOBJ 5ull /**< couldn't find internal t_cmmjl_obj */
#define CMMJL_ENOMEM 6ull /**< out of memory */

/* verbose general errors */
#define CMMJL_ENOFUNC 10ull /**< no such function */

/* OSC errors */
#define	CMMJL_OSC_ENO4BYTE 0x100ull /**< OSC packet is not a multiple of 4 bytes long */
#define	CMMJL_OSC_EUNDRFLW 0x200ull/**< data in OSC packet is too small */
#define	CMMJL_OSC_EOVRFLW 0x300ull /**< data in OSC packet is too big */
#define	CMMJL_OSC_EBNDLNO4 0x400ull /**< OSC bundle size is not a multiple of 4 bytes long */
#define	CMMJL_OSC_EBADBNDL 0x500ull /**< OSC bad bundle size */
#define	CMMJL_OSC_EBADMSG 0x600ull /**< OSC bad message name */
#define CMMJL_OSC_ETYPTAG 0x700ull /**< unrecognized OSC type tag */
#define CMMJL_OSC_EARGS 0x800ull /**< too many arguments in OSC message */
#define CMMJL_OSC_EBADALIGN 0x900ull /**< bad string alignment in OSC packet */
#define CMMJL_OSC_ELIB 0xA00ull /**< the OSC lib encountered an error while processing a packet */

/* verbose OSC errors */
#define CMMJL_OSC_EMATCH 0x1000ull /**< A problem was encountered will attempting a match */
#define CMMJL_OSC_ELATE 0x2000ull /**< OSC packet arrived too late for scheduling */
#define CMMJL_OSC_EMAXDEL 0x3000ull /**< OSC packet was scheduled too far into the future */
/*@}*/

#endif

/*@}*/
