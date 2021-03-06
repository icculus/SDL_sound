/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
*/

//////////////////////////////////////////////
// PTM PolyTracker module loader            //
//////////////////////////////////////////////
#include "libmodplug.h"

#pragma pack(1)

typedef struct PTMFILEHEADER
{
	CHAR songname[28];		// name of song, asciiz string
	CHAR eof;				// 26
	BYTE version_lo;		// 03 version of file, currently 0203h
	BYTE version_hi;		// 02
	BYTE reserved1;			// reserved, set to 0
	WORD norders;			// number of orders (0..256)
	WORD nsamples;			// number of instruments (1..255)
	WORD npatterns;			// number of patterns (1..128)
	WORD nchannels;			// number of channels (voices) used (1..32)
	WORD fileflags;			// set to 0
	WORD reserved2;			// reserved, set to 0
	DWORD ptmf_id;			// song identification, 'PTMF' or 0x464d5450
	BYTE reserved3[16];		// reserved, set to 0
	BYTE chnpan[32];		// channel panning settings, 0..15, 0 = left, 7 = middle, 15 = right
	BYTE orders[256];		// order list, valid entries 0..nOrders-1
	WORD patseg[128];		// pattern offsets (*16)
} PTMFILEHEADER, *LPPTMFILEHEADER;

#define SIZEOF_PTMFILEHEADER	608


typedef struct PTMSAMPLE
{
	BYTE sampletype;		// sample type (bit array)
	CHAR filename[12];		// name of external sample file
	BYTE volume;			// default volume
	WORD nC4Spd;			// C4 speed
	WORD sampleseg;			// sample segment (used internally)
	WORD fileofs[2];		// offset of sample data
	WORD length[2];			// sample size (in bytes)
	WORD loopbeg[2];		// start of loop
	WORD loopend[2];		// end of loop
	WORD gusdata[8];
	char  samplename[28];	// name of sample, asciiz  // changed from CHAR
	DWORD ptms_id;			// sample identification, 'PTMS' or 0x534d5450
} PTMSAMPLE;

#define SIZEOF_PTMSAMPLE	80

#pragma pack()


static uint32_t BS2WORD(uint16_t w[2]) {
	uint32_t u32 = (w[1] << 16) + w[0];
	return(bswapLE32(u32));
}

BOOL CSoundFile_ReadPTM(CSoundFile *_this, const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	PTMFILEHEADER pfh;
	DWORD dwMemPos;
	UINT nOrders;
	UINT j;

	if ((!lpStream) || (dwMemLength < sizeof(PTMFILEHEADER))) return FALSE;
	SDL_memcpy(&pfh, lpStream, sizeof (pfh));

	pfh.norders = bswapLE16(pfh.norders);
	pfh.nsamples = bswapLE16(pfh.nsamples);
	pfh.npatterns = bswapLE16(pfh.npatterns);
	pfh.nchannels = bswapLE16(pfh.nchannels);
	pfh.fileflags = bswapLE16(pfh.fileflags);
	pfh.reserved2 = bswapLE16(pfh.reserved2);
	pfh.ptmf_id = bswapLE32(pfh.ptmf_id);
	for (j=0; j<128; j++)
        {
	        pfh.patseg[j] = bswapLE16(pfh.patseg[j]);
	}

	if ((pfh.ptmf_id != 0x464d5450) || (!pfh.nchannels)
	 || (pfh.nchannels > 32)
	 || (pfh.norders > 256) || (!pfh.norders)
	 || (!pfh.nsamples) || (pfh.nsamples > 255)
	 || (!pfh.npatterns) || (pfh.npatterns > 128)
	 || (SIZEOF_PTMFILEHEADER+pfh.nsamples*SIZEOF_PTMSAMPLE >= (int)dwMemLength)) return FALSE;
	_this->m_nType = MOD_TYPE_PTM;
	_this->m_nChannels = pfh.nchannels;
	_this->m_nSamples = (pfh.nsamples < MAX_SAMPLES) ? pfh.nsamples : MAX_SAMPLES-1;
	dwMemPos = SIZEOF_PTMFILEHEADER;
	nOrders = (pfh.norders < MAX_ORDERS) ? pfh.norders : MAX_ORDERS-1;
	SDL_memcpy(_this->Order, pfh.orders, nOrders);
	for (j=0; j<_this->m_nChannels; j++)
	{
		_this->ChnSettings[j].nVolume = 64;
		_this->ChnSettings[j].nPan = ((pfh.chnpan[j] & 0x0F) << 4) + 4;
	}
	for (j=0; j<_this->m_nSamples; j++, dwMemPos += SIZEOF_PTMSAMPLE)
	{
		MODINSTRUMENT *pins = &_this->Ins[j+1];
		PTMSAMPLE *psmp = (PTMSAMPLE *)(lpStream+dwMemPos);

		pins->nGlobalVol = 64;
		pins->nPan = 128;
		pins->nVolume = psmp->volume << 2;
		pins->nC4Speed = bswapLE16(psmp->nC4Spd) << 1;
		pins->uFlags = 0;
		if ((psmp->sampletype & 3) == 1)
		{
			UINT smpflg = RS_PCM8D;
			DWORD samplepos;
			pins->nLength = BS2WORD(psmp->length);
			pins->nLoopStart = BS2WORD(psmp->loopbeg);
			pins->nLoopEnd = BS2WORD(psmp->loopend);
			samplepos = BS2WORD(psmp->fileofs);
			if (psmp->sampletype & 4) pins->uFlags |= CHN_LOOP;
			if (psmp->sampletype & 8) pins->uFlags |= CHN_PINGPONGLOOP;
			if (psmp->sampletype & 16)
			{
				pins->uFlags |= CHN_16BIT;
				pins->nLength >>= 1;
				pins->nLoopStart >>= 1;
				pins->nLoopEnd >>= 1;
				smpflg = RS_PTM8DTO16;
			}
			if ((pins->nLength) && (samplepos) && (samplepos < dwMemLength))
			{
				CSoundFile_ReadSample(_this, pins, smpflg, (LPSTR)(lpStream+samplepos), dwMemLength-samplepos);
			}
		}
	}
	// Reading Patterns
	for (j=0; j<pfh.npatterns; j++)
	{
		MODCOMMAND *m;
		UINT row;
		dwMemPos = ((UINT)pfh.patseg[j]) << 4;
		if ((!dwMemPos) || (dwMemPos >= dwMemLength)) continue;
		_this->PatternSize[j] = 64;
		if ((_this->Patterns[j] = CSoundFile_AllocatePattern(64, _this->m_nChannels)) == NULL) break;
		//
		m = _this->Patterns[j];
		for (row=0; ((row < 64) && (dwMemPos < dwMemLength)); )
		{
			UINT b = lpStream[dwMemPos++];

			if (dwMemPos >= dwMemLength) break;
			if (b)
			{
				UINT nChn = b & 0x1F;
				MODCOMMAND *selm = &m[nChn < _this->m_nChannels ? nChn : 0];

				if (b & 0x20)
				{
					if (dwMemPos + 2 > dwMemLength) break;
					selm->note = lpStream[dwMemPos++];
					selm->instr = lpStream[dwMemPos++];
				}
				if (b & 0x40)
				{
					if (dwMemPos + 2 > dwMemLength) break;
					selm->command = lpStream[dwMemPos++];
					selm->param = lpStream[dwMemPos++];
					if ((selm->command == 0x0E) && ((selm->param & 0xF0) == 0x80))
					{
						selm->command = CMD_S3MCMDEX;
					} else
					if (selm->command < 0x10)
					{
						CSoundFile_ConvertModCommand(_this, selm);
					} else
					{
						switch(selm->command)
						{
						case 16:
							selm->command = CMD_GLOBALVOLUME;
							break;
						case 17:
							selm->command = CMD_RETRIG;
							break;
						case 18:
							selm->command = CMD_FINEVIBRATO;
							break;
						default:
							selm->command = 0;
						}
					}
				}
				if (b & 0x80)
				{
					if (dwMemPos >= dwMemLength) break;
					selm->volcmd = VOLCMD_VOLUME;
					selm->vol = lpStream[dwMemPos++];
				}
			} else
			{
				row++;
				m += _this->m_nChannels;
			}
		}
	}
	return TRUE;
}
