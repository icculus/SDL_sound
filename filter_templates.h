/*
    Extended Audio Converter for SDL (Simple DirectMedia Layer)
    Copyright (C) 2002  Frank Ranostaj
                        Institute of Applied Physik
                        Johann Wolfgang Goethe-Universität
                        Frankfurt am Main, Germany

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Frank Ranostaj
    ranostaj@stud.uni-frankfurt.de
*/

#ifndef Suffix
#error include filter_template.h with defined Suffix macro!
#else
#define CH(x) (Suffix((x)*))
//---------------------------------------------------------------------------
int Suffix(_doubleRate)( short *buffer, int mode, int length )
{
    const fsize = _fsize/2;
    int i,di,border;
    short inbuffer[_fsize];

    if( mode & SDL_AI_Loop )
    {
        for( i = 0; i < fsize; i++ )
        {
            inbuffer[CH(i+fsize)] = buffer[CH(length+i)] = buffer[CH(i)];
            inbuffer[CH(i)] = buffer[CH(length-fsize+i)];
        }
        border = 0;
    }
    else
    {
        for( i = 0; i < fsize; i++ )
        {
            inbuffer[CH(i)] = buffer[CH(length+i)] = 0;
            inbuffer[CH(i+fsize)] = buffer[CH(i)];
        }
        border = fsize/2;
    }

    for(i = length + border - 1; i >= -border; i--)
    {
        const short* const inp = i < fsize/2 ?
                                 &inbuffer[CH(i+fsize)] : &buffer[CH(i)];
        short* const outp = &buffer[CH(2*(i+border))];
        int out = 0;

        for( di = 1; di < 1+fsize; di+=2 )
            out+= filter[di]*( inp[CH(di)/2] + inp[CH(1-di)/2] );
        outp[CH(1)] = ( 32770*inp[CH(1)] + out) >> 16;
        outp[CH(0)] = ( 32770*inp[CH(0)] + out) >> 16;
    }
    return  2*length + 4*border;
}

//---------------------------------------------------------------------------
short Suffix(filterHalfBand)( short* inp )
{
    static const int fsize = _fsize;
    int out = 32770*inp[0];
    int di;
    for( di = 1; di < fsize/2; di+=2 )
    	out+= filter[di]*( inp[CH(di)] + inp[CH(-di)] );
    return out >> 16;
}

int Suffix(_halfRate)( short *buffer, int mode, int length )
{
    static const int fsize = _fsize;

    int i,border;

    short inbuffer[3*_fsize];
    short *finp, *linp;

    if( mode & SDL_AI_Loop )
    {
        if( length & 1 )
        {
        // do something meaningful
        }
        for( i = 0; i < fsize; i++ )
        {
            inbuffer[CH(i)] = buffer[CH(length-fsize+i)];
            inbuffer[CH(i+fsize)] = buffer[CH(i)];
        }
        border = 0;
        finp = inbuffer + CH( fsize );
        linp = inbuffer + CH( fsize-length );
    }
    else
    {
        for( i = 0; i < fsize; i++ )
        {
            inbuffer[CH(i)] = buffer[CH(length-fsize+i)];
            inbuffer[CH(i+fsize)] = 0;
            inbuffer[CH(i+2*fsize)] = buffer[CH(i)];
        }
        border = fsize;
        finp = inbuffer + CH( (3*fsize)/2 + 2*border );
        linp = inbuffer + CH( fsize/2 - length );
    }

    length = ( length + 1 ) / 2;

    for(i = -border; i < fsize; i++)
    {
        buffer[CH(i+border)] = Suffix(filterHalfBand)( finp+CH(2*i) );
    }
    for(; i < length-fsize; i++)
    {
        buffer[CH(i+border)] = Suffix(filterHalfBand)( buffer+CH(2*i) );
    }
    for(; i < length+border; i++)
    {
        buffer[CH(i+border)] = Suffix(filterHalfBand)( linp+CH(2*i) );
    }
    return length + 2*border;
}

//---------------------------------------------------------------------------
short Suffix(filterVarBand)( VarFilter* filt, short** inpp, char* cpos )
{
    int di;
    int out = 0;
    short *inp = *inpp;
    int pos = *cpos;
    short *filter = filt->c[pos];

    for( di = 0; di < 2*_fsize; di++ )
    	out+= filter[di] * (int)inp[CH(di)];

    *inpp += CH(filt->incr[pos]);
    *cpos = ( pos + 1 ) % filt->pos_mod;
    return out >> 16;
}

int Suffix(_varRateDown)( short* buffer, int mode, VarFilter* filter, int length )
{
    static const int fsize = _fsize;
    int i,border;
    short inbuffer[CH(3*_fsize)];
    short *finp, *linp, *bufp, *outbuf;
    char pos = 0;
    VarFilter* filterp = filter;

    if( mode & SDL_AI_Loop )
    {
        for( i = 0; i < fsize; i++ )
        {
            inbuffer[CH(i)] = buffer[CH(length-fsize+i)];
            inbuffer[CH(i+fsize)] = buffer[CH(i)];
        }
        border = 0;
        finp = inbuffer+CH(fsize);
        linp = inbuffer+CH(fsize-length);
    }
    else
    {
        for( i = 0; i < fsize; i++ )
        {
            inbuffer[CH(i)] = buffer[CH(length-fsize+i)];
            inbuffer[CH(i+fsize)] = 0;
            inbuffer[CH(i+2*fsize)] = buffer[CH(i)];
        }
        border = fsize;
        finp = inbuffer + CH( 3*fsize/2 );
        linp = inbuffer + CH( fsize/2 );
    }

    length = ( length + 1 ) / 2;
    bufp = buffer;
    outbuf = buffer+CH(border);

    for(i = -border; i < fsize; i++)
    {
        outbuf[CH(i)] = Suffix(filterVarBand)( filterp, &finp, &pos );
    }
    for(; i < length-fsize; i++)
    {
        outbuf[CH(i)] = Suffix(filterVarBand)( filterp, &bufp, &pos );
    }
    for(; i < length+border; i++)
    {
        outbuf[CH(i)] = Suffix(filterVarBand)( filterp, &linp, &pos );
    }
    return length + 2*border;
}

int Suffix(_varRateUp)( short* buffer, int mode, VarFilter* filter, int length )
{
    static const int fsize = _fsize;
    int i,border;
    short inbuffer[CH(3*_fsize)];
    short *finp, *linp, *bufp, *outbuf;
    char pos = 0;
    VarFilter* filterp = filter;

    if( mode & SDL_AI_Loop )
    {
        for( i = 0; i < fsize; i++ )
        {
            inbuffer[CH(i)] = buffer[CH(length-fsize+i)];
            inbuffer[CH(i+fsize)] = buffer[CH(i)];
        }
        border = 0;
        finp = inbuffer+CH(fsize);
        linp = inbuffer+CH(fsize-length);
    }
    else
    {
        for( i = 0; i < fsize; i++ )
        {
            inbuffer[CH(i)] = buffer[CH(length-fsize+i)];
            inbuffer[CH(i+fsize)] = 0;
            inbuffer[CH(i+2*fsize)] = buffer[CH(i)];
        }
        border = fsize;
        finp = inbuffer + CH( 3*fsize/2 );
        linp = inbuffer + CH( fsize/2 );
    }

    length = 2 * length;
    bufp = buffer+length;
    outbuf = buffer+CH(border);

    for(i = length+border-1; i > length-fsize-1; i--)
    {
        outbuf[CH(i)] = Suffix(filterVarBand)( filterp, &linp, &pos );
    }
    for(; i > fsize-1 ; i--)
    {
        outbuf[CH(i)] = Suffix(filterVarBand)( filterp, &bufp, &pos );
    }
    for(; i > -border-1; i--)
    {
        outbuf[CH(i)] = Suffix(filterVarBand)( filterp, &finp, &pos );
    }
    return length + 2*border;
}
//---------------------------------------------------------------------------
#undef CH
#endif /* Suffix */

