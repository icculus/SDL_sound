/*
 * This source code is public domain.
 *
 * Authors: Kenton Varda <temporal@gauge3d.org> (C interface wrapper)
 */

#include "modplug.h"
#include "libmodplug.h"

struct _ModPlugFile
{
	CSoundFile mSoundFile;
};

namespace ModPlug
{
	ModPlug_Settings gSettings =
	{
		MODPLUG_ENABLE_OVERSAMPLING | MODPLUG_ENABLE_NOISE_REDUCTION,

		2, // mChannels
		16, // mBits
		44100, // mFrequency
		MODPLUG_RESAMPLE_LINEAR, //mResamplingMode

		128, // mStereoSeparation
		32, // mMaxMixChannels
		0,
		0,
		0,
		0,
		0,
		0,
		0
	};

	int gSampleSize;

	void UpdateSettings(bool updateBasicConfig)
	{
		if(gSettings.mFlags & MODPLUG_ENABLE_REVERB)
		{
			CSoundFile::SetReverbParameters(gSettings.mReverbDepth,
			                                gSettings.mReverbDelay);
		}

		if(gSettings.mFlags & MODPLUG_ENABLE_MEGABASS)
		{
			CSoundFile::SetXBassParameters(gSettings.mBassAmount,
			                               gSettings.mBassRange);
		}
		else // modplug seems to ignore the SetWaveConfigEx() setting for bass boost
			CSoundFile::SetXBassParameters(0, 0);

		if(gSettings.mFlags & MODPLUG_ENABLE_SURROUND)
		{
			CSoundFile::SetSurroundParameters(gSettings.mSurroundDepth,
			                                  gSettings.mSurroundDelay);
		}

		if(updateBasicConfig)
		{
			CSoundFile::SetWaveConfig(gSettings.mFrequency,
                                                  gSettings.mBits,
			                          gSettings.mChannels);
			CSoundFile::SetMixConfig(gSettings.mStereoSeparation,
                                                 gSettings.mMaxMixChannels);

			gSampleSize = gSettings.mBits / 8 * gSettings.mChannels;
		}

		CSoundFile::SetWaveConfigEx(gSettings.mFlags & MODPLUG_ENABLE_SURROUND,
		                            !(gSettings.mFlags & MODPLUG_ENABLE_OVERSAMPLING),
		                            gSettings.mFlags & MODPLUG_ENABLE_REVERB,
		                            true,
		                            gSettings.mFlags & MODPLUG_ENABLE_MEGABASS,
		                            gSettings.mFlags & MODPLUG_ENABLE_NOISE_REDUCTION,
		                            false);
		CSoundFile::SetResamplingMode(gSettings.mResamplingMode);
	}
}

ModPlugFile* ModPlug_Load(const void* data, int size)
{
	ModPlugFile* result = new ModPlugFile;
	ModPlug::UpdateSettings(true);
	if(result->mSoundFile.Create((const BYTE*)data, size))
	{
		result->mSoundFile.SetRepeatCount(ModPlug::gSettings.mLoopCount);
		return result;
	}
	else
	{
		delete result;
		return NULL;
	}
}

void ModPlug_Unload(ModPlugFile* file)
{
	file->mSoundFile.Destroy();
	delete file;
}

int ModPlug_Read(ModPlugFile* file, void* buffer, int size)
{
	return file->mSoundFile.Read(buffer, size) * ModPlug::gSampleSize;
}

int ModPlug_GetLength(ModPlugFile* file)
{
	return file->mSoundFile.GetSongTime() * 1000;
}

void ModPlug_Seek(ModPlugFile* file, int millisecond)
{
	int maxpos;
	int maxtime = file->mSoundFile.GetSongTime() * 1000;
	float postime;

	if(millisecond > maxtime)
		millisecond = maxtime;
	maxpos = file->mSoundFile.GetMaxPosition();
	postime = 0.0f;
	if (maxtime != 0.0f)
		postime = (float)maxpos / (float)maxtime;

	file->mSoundFile.SetCurrentPos((int)(millisecond * postime));
}

void ModPlug_SetSettings(const ModPlug_Settings* settings)
{
	SDL_memcpy(&ModPlug::gSettings, settings, sizeof(ModPlug_Settings));
	ModPlug::UpdateSettings(false); // do not update basic config.
}

// inefficient, but oh well.
char *rwops_fgets(char *buf, int buflen, SDL_RWops *rwops)
{
    char *retval = buf;
    if (!buflen) return buf;
    while (buflen > 1) {
        char ch;
        if (SDL_RWread(rwops, &ch, 1, 1) != 1) {
            break;
        }
        *(buf++) = ch;
        buflen--;
        if (ch == '\n') {
            break;
        }
    }
    *(buf) = '\0';
    return retval;
}

long mmftell(MMFILE *mmfile)
{
	return mmfile->pos;
}

void mmfseek(MMFILE *mmfile, long p, int whence)
{
	int newpos = mmfile->pos;
	switch(whence) {
		case SEEK_SET:
			newpos = p;
			break;
		case SEEK_CUR:
			newpos += p;
			break;
		case SEEK_END:
			newpos = mmfile->sz + p;
			break;
	}
	if (newpos < mmfile->sz)
		mmfile->pos = newpos;
	else {
		mmfile->error = 1;
//		printf("WARNING: seeking too far\n");
	}
}

void mmreadUBYTES(BYTE *buf, long sz, MMFILE *mmfile)
{
	int sztr = sz;
	// do not overread.
	if (sz > mmfile->sz - mmfile->pos)
		sztr = mmfile->sz - mmfile->pos;
	SDL_memcpy(buf, &mmfile->mm[mmfile->pos], sztr);
	mmfile->pos += sz;
	// if truncated read, populate the rest of the array with zeros.
	if (sz > sztr)
		SDL_memset(buf+sztr, 0, sz-sztr);
}

void mmreadSBYTES(char *buf, long sz, MMFILE *mmfile)
{
	// do not overread.
	if (sz > mmfile->sz - mmfile->pos)
		sz = mmfile->sz - mmfile->pos;
	SDL_memcpy(buf, &mmfile->mm[mmfile->pos], sz);
	mmfile->pos += sz;
}

BYTE mmreadUBYTE(MMFILE *mmfile)
{
	BYTE b;
	b = (BYTE)mmfile->mm[mmfile->pos];
	mmfile->pos++;
	return b;
}

void mmfclose(MMFILE *mmfile)
{
	SDL_free(mmfile);
}

bool mmfeof(MMFILE *mmfile)
{
	if( mmfile->pos < 0 ) return TRUE;
	if( mmfile->pos < mmfile->sz ) return FALSE;
	return TRUE;
}

int mmfgetc(MMFILE *mmfile)
{
	int b = EOF;
	if( mmfeof(mmfile) ) return EOF;
	b = mmfile->mm[mmfile->pos];
	mmfile->pos++;
	if( b=='\r' && !mmfeof(mmfile) && mmfile->mm[mmfile->pos] == '\n' ) {
		b = '\n';
		mmfile->pos++;
	}
	return b;
}

void mmfgets(char buf[], unsigned int bufsz, MMFILE *mmfile)
{
	int i = 0;
    int b;
	for( i=0; i<(int)bufsz-1; i++ ) {
		b = mmfgetc(mmfile);
		if( b==EOF ) break;
		buf[i] = b;
		if( b == '\n' ) break;
	}
	buf[i] = '\0';
}

