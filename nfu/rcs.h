/*
 * rcs.h -- a way to put RCS IDs into executables.
 *
 * Written by Huadong Liu, http://www.cs.utk.edu/~hliu/
 * 
 * @(#) $Id: rcs.h,v 1.2 2005/09/21 15:42:49 hliu Exp $ 
 */

#ifndef RCS_H_INCLUDED
#define RCS_H_INCLUDED

#define RCS_ID(id) static const char *rcs_id = id;  \
    static const char *                             \
    f_rcs_id(const char *s)                         \
    {                                               \
        if (s) return s;                            \
        else return f_rcs_id(rcs_id);               \
    }

#endif /* ! RCS_H_INCLUDED */
