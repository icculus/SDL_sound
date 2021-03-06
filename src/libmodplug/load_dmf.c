/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

///////////////////////////////////////////////////////
// DMF DELUSION DIGITAL MUSIC FILEFORMAT (X-Tracker) //
///////////////////////////////////////////////////////
#include "libmodplug.h"

#pragma pack(1)

typedef struct DMFHEADER
{
	DWORD id;				// "DDMF" = 0x464d4444
	BYTE version;			// 4
	CHAR trackername[8];	// "XTRACKER"
	CHAR songname[30];
	CHAR composer[20];
	BYTE date[3];
} DMFHEADER;

typedef struct DMFINFO
{
	DWORD id;			// "INFO"
	DWORD infosize;
} DMFINFO;

typedef struct DMFSEQU
{
	DWORD id;			// "SEQU"
	DWORD seqsize;
	WORD loopstart;
	WORD loopend;
	WORD sequ[2];
} DMFSEQU;

typedef struct DMFPATT
{
	DWORD id;			// "PATT"
	DWORD patsize;
	WORD numpat;		// 1-1024
	BYTE tracks;
	BYTE firstpatinfo;
} DMFPATT;

typedef struct DMFTRACK
{
	BYTE tracks;
	BYTE beat;		// [hi|lo] -> hi=ticks per beat, lo=beats per measure
	WORD ticks;		// max 512
	DWORD jmpsize;
} DMFTRACK;

typedef struct DMFSMPI
{
	DWORD id;
	DWORD size;
	BYTE samples;
} DMFSMPI;

typedef struct DMFSAMPLE
{
	DWORD len;
	DWORD loopstart;
	DWORD loopend;
	WORD c3speed;
	BYTE volume;
	BYTE flags;
} DMFSAMPLE;

#pragma pack()


BOOL CSoundFile_ReadDMF(CSoundFile *_this, const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	const DMFHEADER *pfh = (DMFHEADER *)lpStream;
	const DMFINFO *psi;
	const DMFPATT *patt;
	const DMFSEQU *sequ;
	DWORD dwMemPos;
	BYTE infobyte[32];
	BYTE smplflags[MAX_SAMPLES], hasSMPI = 0, hasSMPD = 0;
	UINT i;

	if ((!lpStream) || (dwMemLength < 1024)) return FALSE;
	if ((pfh->id != 0x464d4444) || (!pfh->version) || (pfh->version & 0xF0)) return FALSE;
	dwMemPos = 66;
	_this->m_nType = MOD_TYPE_DMF;
	_this->m_nChannels = 0;
	while (dwMemPos < dwMemLength - 7)
	{
		DWORD id = *((LPDWORD)(lpStream+dwMemPos));

		switch(id)
		{
		// "INFO"
		case 0x4f464e49:
		// "CMSG"
		case 0x47534d43:
			psi = (DMFINFO *)(lpStream+dwMemPos);
			if (id == 0x47534d43) dwMemPos++;
			if ((psi->infosize > dwMemLength) || (dwMemPos + 8 > dwMemLength - psi->infosize)) goto dmfexit;
			dwMemPos += psi->infosize + 8 - 1;
			break;

		// "SEQU"
		case 0x55514553:
			sequ = (DMFSEQU *)(lpStream+dwMemPos);
			if ((sequ->seqsize >= dwMemLength) || (dwMemPos + 8 > dwMemLength - sequ->seqsize)) goto dmfexit;
			if (sequ->seqsize >= 4)
			{
				UINT nseq = (sequ->seqsize - 4) >> 1;
				if (nseq >= MAX_ORDERS-1) nseq = MAX_ORDERS-1;
				if (sequ->loopstart < nseq) _this->m_nRestartPos = sequ->loopstart;
				for (i=0; i<nseq; i++) _this->Order[i] = (BYTE)sequ->sequ[i];
			}
			dwMemPos += sequ->seqsize + 8;
			break;

		// "PATT"
		case 0x54544150:
			patt = (DMFPATT *)(lpStream+dwMemPos);
			if ((patt->patsize >= dwMemLength) || (dwMemPos + 8 > dwMemLength - patt->patsize)) goto dmfexit;
			if (patt->patsize >= 4 && !_this->m_nChannels)
			{
				UINT numpat, c;
				DWORD dwPos = dwMemPos + 11;
				numpat = patt->numpat;
				if (numpat > MAX_PATTERNS) numpat = MAX_PATTERNS;
				_this->m_nChannels = patt->tracks;
				if (_this->m_nChannels < patt->firstpatinfo) _this->m_nChannels = patt->firstpatinfo;
				if (_this->m_nChannels > 32) _this->m_nChannels = 32;
				if (_this->m_nChannels < 4) _this->m_nChannels = 4;
				for (c=0; c<numpat; c++)
				{
					const DMFTRACK *pt = (DMFTRACK *)(lpStream+dwPos);
					MODCOMMAND *m;
					UINT tracks, ticks, row;
					UINT ttype, tempo, glbinfobyte, pbeat;
					BOOL tempochange;
					DWORD d;
					if (dwPos + 8 >= dwMemLength) break;
					tracks = pt->tracks;
					if (tracks > 32) tracks = 32;
					ticks = pt->ticks;
					if (ticks > 256) ticks = 256;
					if (ticks < 16) ticks = 16;
					dwPos += 8;
					if ((pt->jmpsize >= dwMemLength) || (dwPos + 4 > dwMemLength - pt->jmpsize)) break;
					_this->PatternSize[c] = (WORD)ticks;
					m = CSoundFile_AllocatePattern(_this->PatternSize[c], _this->m_nChannels);
					if (!m) goto dmfexit;
					_this->Patterns[c] = m;
					d = dwPos;
					dwPos += pt->jmpsize;
					ttype = 1;
					tempo = 125;
					glbinfobyte = 0;
					pbeat = (pt->beat & 0xf0) ? pt->beat>>4 : 8;
					tempochange = (pt->beat & 0xf0) ? TRUE : FALSE;
					SDL_memset(infobyte, 0, sizeof(infobyte));
					for (row=0; row<ticks; row++)
					{
						MODCOMMAND *p = &m[row*_this->m_nChannels];
						// Parse track global effects
						if (!glbinfobyte)
						{
							BYTE info, infoval;
							if (d+1 > dwPos) break;
							info = lpStream[d++];
							infoval = 0;
							if ((info & 0x80) && (d < dwPos)) glbinfobyte = lpStream[d++];
							info &= 0x7f;
							if ((info) && (d < dwPos)) infoval = lpStream[d++];
							switch(info)
							{
							case 1:	ttype = 0; tempo = infoval; tempochange = TRUE; break;
							case 2: ttype = 1; tempo = infoval; tempochange = TRUE; break;
							case 3: pbeat = infoval>>4; tempochange = ttype; break;
							}
						} else
						{
							glbinfobyte--;
						}
						// Parse channels
						for (i=0; i<tracks; i++) if (!infobyte[i])
						{
							MODCOMMAND cmd;
							BYTE info;
							if (d+1 > dwPos) break;
							SDL_zero(cmd);
							info = lpStream[d++];
							if (info & 0x80)
							{
								if (d+1 > dwPos) break;
								infobyte[i] = lpStream[d++];
							}
							// Instrument
							if (info & 0x40)
							{
								if (d+1 > dwPos) break;
								cmd.instr = lpStream[d++];
							}
							// Note
							if (info & 0x20)
							{
								if (d+1 > dwPos) break;
								cmd.note = lpStream[d++];
								if ((cmd.note) && (cmd.note < 0xfe)) cmd.note &= 0x7f;
								if ((cmd.note) && (cmd.note < 128)) cmd.note += 24;
							}
							// Volume
							if (info & 0x10)
							{
								if (d+1 > dwPos) break;
								cmd.volcmd = VOLCMD_VOLUME;
								cmd.vol = (lpStream[d++]+3)>>2;
							}
							// Effect 1
							if (info & 0x08)
							{
								BYTE efx, eval;
								if (d+2 > dwPos) break;

								efx = lpStream[d++];
								eval = lpStream[d++];
								switch(efx)
								{
								// 1: Key Off
								case 1: if (!cmd.note) cmd.note = 0xFE; break;
								// 2: Set Loop
								// 4: Sample Delay
								case 4: if (eval&0xe0) { cmd.command = CMD_S3MCMDEX; cmd.param = (eval>>5)|0xD0; } break;
								// 5: Retrig
								case 5: if (eval&0xe0) { cmd.command = CMD_RETRIG; cmd.param = (eval>>5); } break;
								// 6: Offset
								case 6: cmd.command = CMD_OFFSET; cmd.param = eval; break;
								}
							}
							// Effect 2
							if (info & 0x04)
							{
								BYTE efx, eval;
								if (d+2 > dwPos) break;

								efx = lpStream[d++];
								eval = lpStream[d++];
								switch(efx)
								{
								// 1: Finetune
								case 1: if (eval&0xf0) { cmd.command = CMD_S3MCMDEX; cmd.param = (eval>>4)|0x20; } break;
								// 2: Note Delay
								case 2: if (eval&0xe0) { cmd.command = CMD_S3MCMDEX; cmd.param = (eval>>5)|0xD0; } break;
								// 3: Arpeggio
								case 3: if (eval) { cmd.command = CMD_ARPEGGIO; cmd.param = eval; } break;
								// 4: Portamento Up
								case 4: cmd.command = CMD_PORTAMENTOUP; cmd.param = (eval >= 0xe0) ? 0xdf : eval; break;
								// 5: Portamento Down
								case 5: cmd.command = CMD_PORTAMENTODOWN; cmd.param = (eval >= 0xe0) ? 0xdf : eval; break;
								// 6: Tone Portamento
								case 6: cmd.command = CMD_TONEPORTAMENTO; cmd.param = eval; break;
								// 8: Vibrato
								case 8: cmd.command = CMD_VIBRATO; cmd.param = eval; break;
								// 12: Note cut
								case 12: if (eval & 0xe0) { cmd.command = CMD_S3MCMDEX; cmd.param = (eval>>5)|0xc0; }
									 else if (!cmd.note) { cmd.note = 0xfe; }
									 break;
								}
							}
							// Effect 3
							if (info & 0x02)
							{
								BYTE efx, eval;
								if (d+2 > dwPos) break;

								efx = lpStream[d++];
								eval = lpStream[d++];
								switch(efx)
								{
								// 1: Vol Slide Up
								case 1: if (eval == 0xff) break;
										eval = (eval+3)>>2; if (eval > 0x0f) eval = 0x0f;
										cmd.command = CMD_VOLUMESLIDE; cmd.param = eval<<4; break;
								// 2: Vol Slide Down
								case 2:	if (eval == 0xff) break;
										eval = (eval+3)>>2; if (eval > 0x0f) eval = 0x0f;
										cmd.command = CMD_VOLUMESLIDE; cmd.param = eval; break;
								// 7: Set Pan
								case 7: if (!cmd.volcmd) { cmd.volcmd = VOLCMD_PANNING; cmd.vol = (eval+3)>>2; }
										else { cmd.command = CMD_PANNING8; cmd.param = eval; } break;
								// 8: Pan Slide Left
								case 8: eval = (eval+3)>>2; if (eval > 0x0f) eval = 0x0f;
										cmd.command = CMD_PANNINGSLIDE; cmd.param = eval<<4; break;
								// 9: Pan Slide Right
								case 9: eval = (eval+3)>>2; if (eval > 0x0f) eval = 0x0f;
										cmd.command = CMD_PANNINGSLIDE; cmd.param = eval; break;

								}
							}
							// Store effect
							if (i < _this->m_nChannels) p[i] = cmd;
							if (d > dwPos)
							{
								break;
							}
						} else
						{
							infobyte[i]--;
						}

						// Find free channel for tempo change
						if (tempochange)
						{
							UINT speed=6, modtempo=tempo;
							UINT rpm = ((ttype) && (pbeat)) ? tempo*pbeat : (tempo+1)*15;
							tempochange = FALSE;
							for (speed=30; speed>1; speed--)
							{
								modtempo = rpm*speed/24;
								if (modtempo <= 200) break;
								if ((speed < 6) && (modtempo < 256)) break;
							}
							for (i=0; i<_this->m_nChannels; i++) if (!p[i].command)
							{
								if (speed)
								{
									p[i].command = CMD_SPEED;
									p[i].param = (BYTE)speed;
									speed = 0;
								} else
								if ((modtempo >= 32) && (modtempo < 256))
								{
									p[i].command = CMD_TEMPO;
									p[i].param = (BYTE)modtempo;
									modtempo = 0;
								} else
								{
									break;
								}
							}
						}
						if (d >= dwPos) break;
					}
					if (dwPos + 8 >= dwMemLength) break;
				}
			}
			dwMemPos += patt->patsize + 8;
			break;

		// "SMPI": Sample Info
		case 0x49504d53:
			{
				const DMFSMPI *pds = (DMFSMPI *)(lpStream+dwMemPos);
				hasSMPI = 1;
				if ((pds->size >= dwMemLength) || (dwMemPos + 8 > dwMemLength - pds->size)) goto dmfexit;
				if (pds->size >= 1)
				{
					DWORD dwPos = dwMemPos + 9;
					_this->m_nSamples = pds->samples;
					if (_this->m_nSamples >= MAX_SAMPLES) _this->m_nSamples = MAX_SAMPLES-1;
					for (i=1; i<=_this->m_nSamples; i++)
					{
						const DMFSAMPLE *psh;
						MODINSTRUMENT *psmp;
						UINT namelen;
						if (dwPos >= dwMemPos + pds->size + 8) break;
						namelen = lpStream[dwPos];
						smplflags[i] = 0;
						if (dwPos+namelen+1+sizeof(DMFSAMPLE) > dwMemPos+pds->size+8) break;
						dwPos += namelen + 1;
						psh = (DMFSAMPLE *)(lpStream+dwPos);
						psmp = &_this->Ins[i];
						psmp->nLength = psh->len;
						psmp->nLoopStart = psh->loopstart;
						psmp->nLoopEnd = psh->loopend;
						psmp->nC4Speed = psh->c3speed;
						psmp->nGlobalVol = 64;
						psmp->nVolume = (psh->volume) ? ((WORD)psh->volume)+1 : (WORD)256;
						psmp->uFlags = (psh->flags & 2) ? CHN_16BIT : 0;
						if (psmp->uFlags & CHN_16BIT) psmp->nLength >>= 1;
						if (psh->flags & 1) psmp->uFlags |= CHN_LOOP;
						smplflags[i] = psh->flags;
						dwPos += (pfh->version < 8) ? 22 : 30;
					}
				}
				dwMemPos += pds->size + 8;
			}
			break;

		// "SMPD": Sample Data
		case 0x44504d53:
			{
				DWORD dwPos = dwMemPos + 8;
				UINT ismpd = 0;
				for (i=1; i<=_this->m_nSamples && !hasSMPD; i++)
				{
					DWORD pksize;
					ismpd++;
					if (dwPos + 4 >= dwMemLength)
					{
						break;
					}
					pksize = *((LPDWORD)(lpStream+dwPos));
					dwPos += 4;
					if (pksize > dwMemLength - dwPos)
					{
						pksize = dwMemLength - dwPos;
					}
					if ((pksize) && (i <= _this->m_nSamples))
					{
						UINT flags = (_this->Ins[i].uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
						if (hasSMPI && smplflags[ismpd] & 4)
							flags = (_this->Ins[i].uFlags & CHN_16BIT) ? RS_DMF16 : RS_DMF8;
						CSoundFile_ReadSample(_this, &_this->Ins[i], flags, (LPSTR)(lpStream+dwPos), pksize);
					}
					dwPos += pksize;
				}
				hasSMPD = 1;
				dwMemPos = dwPos;
			}
			break;

		// "ENDE": end of file
		case 0x45444e45:
			goto dmfexit;
		
		// Unrecognized id, or "ENDE" field
		default:
			dwMemPos += 4;
			break;
		}
	}
dmfexit:
	if (!_this->m_nChannels)
	{
		if (!_this->m_nSamples)
		{
			_this->m_nType = MOD_TYPE_NONE;
			return FALSE;
		}
		_this->m_nChannels = 4;
	}
	return TRUE;
}


///////////////////////////////////////////////////////////////////////
// DMF Compression

#pragma pack(1)

typedef struct DMF_HNODE
{
	short int left, right;
	BYTE value;
} DMF_HNODE;

typedef struct DMF_HTREE
{
	LPBYTE ibuf, ibufmax;
	DWORD bitbuf;
	UINT bitnum;
	UINT lastnode, nodecount;
	DMF_HNODE nodes[256];
} DMF_HTREE;

#pragma pack()


// DMF Huffman ReadBits
static BYTE DMFReadBits(DMF_HTREE *tree, UINT nbits)
//-------------------------------------------
{
	BYTE x = 0, bitv = 1;
	while (nbits--)
	{
		if (tree->bitnum)
		{
			tree->bitnum--;
		} else
		if (tree->ibuf < tree->ibufmax) {
			tree->bitbuf = *(tree->ibuf++);
			tree->bitnum = 7;
		}
		if (tree->bitbuf & 1) x |= bitv;
		bitv <<= 1;
		tree->bitbuf >>= 1;
	}
	return x;
}

//
// tree: [8-bit value][12-bit index][12-bit index] = 32-bit
//

static void DMFNewNode(DMF_HTREE *tree)
//------------------------------
{
	BYTE isleft, isright;
	UINT actnode;

	actnode = tree->nodecount;
	if (actnode > 255) return;
	tree->nodes[actnode].value = DMFReadBits(tree, 7);
	isleft = DMFReadBits(tree, 1);
	isright = DMFReadBits(tree, 1);
	actnode = tree->lastnode;
	if (actnode > 255) return;
	tree->nodecount++;
	tree->lastnode = tree->nodecount;
	if (isleft)
	{
		tree->nodes[actnode].left = tree->lastnode;
		DMFNewNode(tree);
	} else
	{
		tree->nodes[actnode].left = -1;
	}
	tree->lastnode = tree->nodecount;
	if (isright)
	{
		tree->nodes[actnode].right = tree->lastnode;
		DMFNewNode(tree);
	} else
	{
		tree->nodes[actnode].right = -1;
	}
}


int DMFUnpack(LPBYTE psample, LPBYTE ibuf, LPBYTE ibufmax, UINT maxlen)
//----------------------------------------------------------------------
{
	DMF_HTREE tree;
	UINT actnode,i;
	BYTE value, sign, delta = 0;

	SDL_memset(&tree, 0, sizeof(tree));
	tree.ibuf = ibuf;
	tree.ibufmax = ibufmax;
	DMFNewNode(&tree);
	value = 0;

	if (tree.ibuf >= ibufmax) return tree.ibuf - ibuf;

	for (i=0; i<maxlen; i++)
	{
		if ((tree.ibuf >= tree.ibufmax) && (!tree.bitnum))
		{
			/* unexpected EOF */
			break;
		}
		actnode = 0;
		sign = DMFReadBits(&tree, 1);
		do
		{
			if (DMFReadBits(&tree, 1))
				actnode = tree.nodes[actnode].right;
			else
				actnode = tree.nodes[actnode].left;
			if (actnode > 255) break;
			delta = tree.nodes[actnode].value;
			if ((tree.ibuf >= tree.ibufmax) && (!tree.bitnum)) break;
		} while ((tree.nodes[actnode].left >= 0) && (tree.nodes[actnode].right >= 0));
		if (sign) delta ^= 0xFF;
		value += delta;
		psample[i] = (i) ? value : 0;
	}
	return tree.ibuf - ibuf;
}
