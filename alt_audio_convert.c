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

(This code blatantly abducted for SDL_sound. Thanks, Frank! --ryan.)
*/

#include "alt_audio_convert.h"

#include <stdlib.h>
#include <math.h>

/*provisorisch*/
#define AUDIO_8 (4)
#define AUDIO_16WRONG (8)
#define AUDIO_FORMAT (AUDIO_8 | AUDIO_16WRONG)
#define AUDIO_SIGN (1)


/*-------------------------------------------------------------------------*/
/* this filter (Kaiser-window beta=6.8) gives a decent -80dB attentuation */
static const int filter[_fsize/2] = {
   0, 20798, 0, -6764, 0, 3863, 0, -2560, 0, 1800, 0, -1295, 0, 936, 0,
-671,
   0,   474, 0,  -326, 0,  217, 0,  -138, 0,   83, 0,   -46, 0,  23, 0,   -9
};

/* Mono (1 channel ) */
#define Suffix(x) x##1
#include "filter_templates.h"
#undef Suffix

/* Stereo (2 channel ) */
#define Suffix(x) x##2
#include "filter_templates.h"
#undef Suffix

/* !!! FIXME: Lose all the "short" vars for "Sint16", etc. */

/*-------------------------------------------------------------------------*/
int DECLSPEC Sound_ConvertAudio( Sound_AudioCVT *Data )
{
   AdapterC Temp;
   int i;

   /* !!! FIXME: Try the looping stuff under certain circumstances? --ryan. */
   int mode = 0;

   /* Make sure there's a converter */
   if( Data == NULL ) {
       SDL_SetError("No converter given");
       return(-1);
   }

   /* Make sure there's data to convert */
   if( Data->buf == NULL ) {
       SDL_SetError("No buffer allocated for conversion");
       return(-1);
   }

   /* Set up the conversion and go! */
   Temp.buffer = (short*)Data->buf;
   Temp.mode = mode;
   Temp.filter = &Data->filter;

   Data->len_cvt = Data->len;
   for( i = 0; Data->adapter[i] != NULL; i++ )
       Data->len_cvt = (*Data->adapter[i])( Temp, Data->len_cvt);

   return(0);
}

/*-------------------------------------------------------------------------*/
int expand8BitTo16Bit( AdapterC Data, int length )
{
   int i;
   char* inp = (char*)Data.buffer-1;
   short* buffer = Data.buffer-1;
   for( i = length; i; i-- )
        buffer[i] = inp[i]<<8;
   return length*2;
}

/*-------------------------------------------------------------------------*/
int swapBytes( AdapterC Data, int length )
{
   int i;
   unsigned short a,b;
   short* buffer = Data.buffer;
   for( i = 0; i < length; i++ )
   {
        a = b = buffer[i];
        a <<= 8;
        b >>= 8;
        buffer[i] = a | b;
   }
   return length;
}

/*-------------------------------------------------------------------------*/
int cut16BitTo8Bit( AdapterC Data, int length )
{
   int i;
   short* inp = Data.buffer-1;
   char* buffer = (char*)Data.buffer-1;
   for( i = 0; i < length; i++ )
        buffer[i] = inp[i]>>8;
   return length/2;
}

/*-------------------------------------------------------------------------*/
int changeSigned( AdapterC Data, int length )
{
   int i;
   short* buffer = Data.buffer;
   for( i = 0; i < length; i++ )
        buffer[i] ^= 0x8000;
   return length;
}

/*-------------------------------------------------------------------------*/
int convertStereoToMono( AdapterC Data, int length )
{
   int i;
   short* buffer = Data.buffer;

    /*
     * !!! FIXME: Can we avoid the division in this loop and just keep
     * !!! FIXME:  a second index variable?  --ryan.
     */
   for( i = 0; i < length;  i+=2 )
        buffer[i/2] = ((int)buffer[i] + buffer[i+1] ) >> 1;
   return length/2;
}

/*-------------------------------------------------------------------------*/
int convertMonoToStereo( AdapterC Data, int length )
{
   int i;
   short* buffer = Data.buffer-2;
   length *= 2;

    /*
     * !!! FIXME: Can we avoid the division in this loop and just keep
     * !!! FIXME:  a second index variable?  --ryan.
     */
   for( i = length; i;  i-=2 )
        buffer[i] = buffer [i+1] = buffer[i/2];
   return length*2;
}

/*-------------------------------------------------------------------------*/
int minus5dB( AdapterC Data, int length )
{
   int i;
   short* buffer = Data.buffer;
   for(i = length; i >= 0; i--)
       buffer[i]= 38084 * buffer[i] >> 16;
   return length;
}

/*-------------------------------------------------------------------------*/
int doubleRateStereo( AdapterC Data, int length )
{
   _doubleRate2( Data.buffer, Data.mode, length/2 );
   return 2*_doubleRate2( Data.buffer+1, Data.mode, length/2 );
}

int doubleRateMono( AdapterC Data, int length )
{
   return _doubleRate1( Data.buffer, Data.mode, length );
}

/*-------------------------------------------------------------------------*/
int halfRateStereo( AdapterC Data, int length )
{
   _halfRate2( Data.buffer, Data.mode, length/2 );
   return 2*_halfRate2( Data.buffer+1, Data.mode, length/2 );
}

int halfRateMono( AdapterC Data, int length )
{
   return _halfRate2( Data.buffer, Data.mode, length );
}

/*-------------------------------------------------------------------------*/
int varRateStereo( AdapterC Data, int length )
{
   _varRate2( Data.buffer, Data.mode, Data.filter, length/2 );
   return 2*_varRate2( Data.buffer+1, Data.mode, Data.filter, length/2 );
}

int varRateMono( AdapterC Data, int length )
{
   return _varRate1( Data.buffer, Data.mode, Data.filter, length );
}

/*-------------------------------------------------------------------------*/
typedef struct{
   short denominator;
   short numerator;
} Fraction;

/*-------------------------------------------------------------------------*/
Fraction findFraction( float Value )
{
/* gives a maximal error of 3% and typical less than 0.2% */
   const char frac[96]={
     1, 2,                                                  -1, /*  /1 */
     1,    3,                                               -1, /*  /2 */
        2,    4, 5,                                         -1, /*  /3 */
           3,    5,    7,                                   -1, /*  /4 */
           3, 4,    6, 7, 8, 9,                             -1, /*  /5 */
                 5,    7,           11,                     -1, /*  /6 */
              4, 5, 6,    8, 9, 10, 11, 12, 13,             -1, /*  /7 */
                 5,    7,    9,     11,     13,     15,     -1, /*  /8 */
                 5,    7, 8,    10, 11,     13, 14,     16, -1, /*  /9 */
                       7,    9,     11,     13,             -1, /* /10 */
                    6, 7, 8, 9, 10,     12, 13, 14, 15, 16, -1, /* /11 */
                       7,           11,     13,             -1, /* /12 */
                       7, 8, 9, 10, 11, 12,     14, 15, 16, -1, /* /13 */
                             9,     11,     13,     15,     -1, /* /14 */
                          8,        11,     13, 14,     16, -1, /* /15 */
                             9,     11,     13,     15       }; /* /16 */


   Fraction Result = {0,0};
   int n,num,den=2;

   float RelErr, BestErr = 0;
   if( Value < 31/64. || Value > 64/31. ) return Result;

   for( n = 0; n < sizeof(frac); num=frac[++n] )
   {
        if( num < 0 ) den++;
        RelErr = Value * num / den;
        RelErr = ( RelErr < (1/RelErr) ? RelErr : 1/RelErr );
        if( RelErr > BestErr )
        {
            BestErr = RelErr;
            Result.denominator = den;
            Result.numerator = num;
        }
   }
   return Result;
}


float sinc( float x )
{
   if( x > -1e-24 && x < 1e-24 ) return 1.;
   else return sin(x)/x;
}

void calculateVarFilter( short* dst, float Ratio, float phase, float scale )
{
   const unsigned short KaiserWindow7[]= {
       22930, 16292, 14648, 14288, 14470, 14945, 15608, 16404,
       17304, 18289, 19347, 20467, 21644, 22872, 24145, 25460,
       26812, 28198, 29612, 31052, 32513, 33991, 35482, 36983,
       38487, 39993, 41494, 42986, 44466, 45928, 47368, 48782,
       50165, 51513, 52821, 54086, 55302, 56466, 57575, 58624,
       59610, 60529, 61379, 62156, 62858, 63483, 64027, 64490,
       64870, 65165, 65375, 65498, 65535, 65484, 65347, 65124,
       64815, 64422, 63946, 63389, 62753, 62039, 61251, 60391 };
   int i;
   float w;
   const float fg = -.018 + .5 / Ratio;
   const float omega = 2 * M_PI * fg;
   phase -= 63;
   for( i = 0; i < 64; i++)
   {
       w = scale * ( KaiserWindow7[i] * ( i + 1 ));
       dst[i] = w * sinc( omega * (i+phase) );
       dst[127-i] = w * sinc( omega * (127-i+phase) );
   }
}

typedef struct{
   float scale;
   int incr;
} VarFilterMode;

const VarFilterMode Up = { 0.0211952, 0 };
const VarFilterMode Down = { 0.0364733, 2 };


void setupVarFilter( VarFilter* filter,
                    float Ratio, VarFilterMode Direction )
{
   int i,n,d;
   Fraction IRatio;
   float phase;
   IRatio = findFraction( Ratio );

   if ( (1/Ratio) < Ratio )
       Ratio = 1/Ratio;

   n = IRatio.numerator;
   d = IRatio.denominator;
   filter->pos_mod = n;

   for( i = 0; i < d; i++ )
   {
       if( phase >= n )
       {
           phase -= d;
           filter->incr[i] = Direction.incr;
       }
       else
           filter->incr[i] = 1;

       calculateVarFilter( filter->c[i], Ratio, phase/(float)n,
                           Direction.scale );
       phase += d;
   }
}

int createRateConverter( Sound_AudioCVT *Data, int filter_index,
                        int SrcRate, int DestRate, int Channel )
{
   int VarPos = 0;
   int Mono = 2 - Channel;
   float Ratio = DestRate;
   if( SrcRate < 1 || SrcRate > 1<<18 ||
       DestRate < 1 || DestRate > 1<<18 ) return -1;
   if( SrcRate == DestRate ) return filter_index;
   Ratio /= SrcRate;

   if( Ratio > 1.0)
       VarPos = filter_index++;
   else
       Data->adapter[filter_index++] = minus5dB;

   while( Ratio > 64.0/31.0)
   {
       Data->adapter[filter_index++] =
           Mono ? doubleRateMono : doubleRateStereo;
       Ratio /= 2;
       Data->len_mult *= 2;
       Data->add *= 2;
       Data->add += _fsize;
   }

   while( Ratio < 31.0/64.0 )
   {
       Data->adapter[filter_index++] =
           Mono ? halfRateMono : halfRateStereo;
       Ratio *= 2;
   }

   if( Ratio > 1.0 )
   {
       setupVarFilter( &Data->filter, Ratio, Up );
       Data->adapter[VarPos] =
           Mono ? varRateMono : varRateStereo;
       Data->len_mult *= 2;
       Data->add *= 2;
       Data->add += _fsize;
   }
   else
   {
       setupVarFilter( &Data->filter, Ratio, Down );
       Data->adapter[filter_index++] =
           Mono ? varRateMono : varRateStereo;
   }
   return 0;
}

int DECLSPEC Sound_BuildAudioCVT(Sound_AudioCVT *Data,
   Uint16 src_format, Uint8 src_channels, int src_rate,
   Uint16 dst_format, Uint8 dst_channels, int dst_rate)
{
   int filter_index = 0;

   if( Data == NULL ) return -1;
   Data->len_mult = 1.0;
   Data->add = 0;

   /* Check channels */
   if( src_channels < 1 || src_channels > 2 ||
       dst_channels < 1 || dst_channels > 2 ) goto error_exit;

   /* First filter:  Size/Endian conversion */
   switch( src_format & AUDIO_FORMAT)
   {
   case AUDIO_8:
       Data->adapter[filter_index++] = expand8BitTo16Bit;
       Data->len_mult *= 2;
       break;
   case AUDIO_16WRONG:
       Data->adapter[filter_index++] = swapBytes;
   }

   /* Second adapter: Sign conversion -- unsigned/signed */
   if( src_format & AUDIO_SIGN )
       Data->adapter[filter_index++] = changeSigned;

   /* Third adapter:  Stereo->Mono conversion */
   if( src_channels == 2 && dst_channels == 1 )
       Data->adapter[filter_index++] = convertStereoToMono;

   /* Do rate conversion */
   if( src_channels == 2 && dst_channels == 2 )
       filter_index = createRateConverter( Data, filter_index,
                                           src_rate, dst_rate, 2 );
   else
       filter_index = createRateConverter( Data, filter_index,
                                           src_rate, dst_rate, 1 );

   if( filter_index < 0 ) goto error_exit; /* propagate error */

   /* adapter: Mono->Stereo conversion */
   if( src_channels == 1 && dst_channels == 2 ){
       Data->adapter[filter_index++] = convertMonoToStereo;
       Data->add *= 2;
       Data->len_mult *= 2;
   }

   /* adapter: final Sign conversion -- unsigned/signed */
   if( dst_format & AUDIO_SIGN )
       Data->adapter[filter_index++] = changeSigned;

   /* final adapter: Size/Endian conversion */
   switch( dst_format & AUDIO_FORMAT)
   {
   case AUDIO_8:
       Data->adapter[filter_index++] = cut16BitTo8Bit;
       break;
   case AUDIO_16WRONG:
       Data->adapter[filter_index++] = swapBytes;
   }
   /* Set up the filter information */
   Data->adapter[filter_index] = NULL;
   Data->needed = (filter_index > 0);
   return 0;

error_exit:
   Data->adapter[0] = NULL;
   return -1;
}
/*-------------------------------------------------------------------------*/

