/*
 *  Extended Audio Converter for SDL (Simple DirectMedia Layer)
 *  Copyright (C) 2002  Frank Ranostaj
 *                      Institute of Applied Physik
 *                      Johann Wolfgang Goethe-Universität
 *                      Frankfurt am Main, Germany
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Frank Ranostaj
 *  ranostaj@stud.uni-frankfurt.de
 *
 * (This code blatantly abducted for SDL_sound. Thanks, Frank! --ryan.)
 */

#ifndef Suffix
#error include filter_template.h with defined Suffix macro!
#else
#define CH(x) (Suffix((x)*))
/*-------------------------------------------------------------------------*/
   /*
    * !!! FIXME !!!
    *
    *
    * Tune doubleRate(), halfRate() and varRate() for speed
    * - Frank
    */

/*-------------------------------------------------------------------------*/
static Sint16* Suffix(doubleRate)( Sint16 *outp, Sint16 *inp, int length,
                                   VarFilter* filt, int* cpos  )
{
    static const int fsize = _fsize;
    int i, out;
    Sint16* to;

    inp += fsize;
    to = inp + length;

    for(; inp > to; inp -= CH(1) )
    {
        out = 0;
        for( i = 1; i < 1+fsize; i++ )
            out+= filter[2*i] * ( inp[CH(i)] + inp[CH(1-i)] );

        outp[CH(1)] = ( 32770 * inp[CH(1)] + out) >> 16;
        outp[CH(0)] = ( 32770 * inp[CH(0)] + out) >> 16;
        outp -= CH(2);
    }
    return outp;
}

/*-------------------------------------------------------------------------*/
static Sint16* Suffix(halfRate)( Sint16 *outp, Sint16 *inp, int length,
                                 VarFilter* filt, int* cpos  )
{
    static const int fsize = CH(_fsize/2);
    int i, out;
    Sint16* to;

    inp -= fsize;
    to = inp + length;

    for(; inp < to; inp+= CH(2) )
    {
        out = 32770 * inp[0];
        for( i = 1; i < fsize/2; i+=2 )
    	    out+= filter[i]*( (int)inp[CH(i)] + inp[CH(-i)] );
        outp[0] = out >> 16;

        outp += CH(1);
    }
    return outp;
}

/*-------------------------------------------------------------------------*/
static Sint16* Suffix(increaseRate)( Sint16 *outp, Sint16 *inp, int length,
                                     VarFilter* filt, int* cpos )
{
    const static int fsize = 2*_fsize;
    Sint16 *filter;
    int out;
    int i, pos;
    Sint16* to;

    inp += fsize;
    to = inp + length;

    while( inp > to )
    {
        pos = *cpos;
        out = 0;
        filter = filt->c[pos];
        for( i = 0; i < 2*_fsize; i++ )
    	    out+= filter[i] * (int)inp[CH(i)];
        outp[0] = out >> 16;

        inp -= CH(filt->incr[pos]);
        outp -= CH(1);
        *cpos = ( pos + 1 ) % filt->denominator;
    }
    return outp;
}

/*-------------------------------------------------------------------------*/
static Sint16* Suffix(decreaseRate)( Sint16 *outp, Sint16 *inp, int length,
                                     VarFilter* filt, int* cpos )
{
    const static int fsize = 2*_fsize;
    Sint16 *filter;
    int out;
    int i, pos;
    Sint16 *to;

    inp -= fsize;
    to = inp + length;

    while( inp < to )
    {
        pos = *cpos;
        out = 0;
        filter = filt->c[pos];
        for( i = 0; i < 2*_fsize; i++ )
    	    out+= filter[i] * (int)inp[CH(i)];
        outp[0] = out >> 16;

        inp += CH(filt->incr[pos]);
        outp += CH(1);
        *cpos = ( pos + 1 ) % filt->denominator;
    }
    return outp;
}

/*-------------------------------------------------------------------------*/
#undef CH
#endif /* Suffix */

