#!/bin/sh

if [ -z $1 ]; then
    echo "USAGE: $0 <new_patch_version>" 1>&2
    exit 1
fi

NEWVERSION="$1"
echo "Updating version to '3.0.$NEWVERSION' ..."

# !!! FIXME: update all of these.

perl -w -pi -e 's/(SDLSOUND_VERSION 3\.0\.)\d+/${1}'$NEWVERSION'/;' CMakeLists.txt
perl -w -pi -e 's/(\#define SOUND_VER_PATCH )\d+/${1}'$NEWVERSION'/;' include/SDL3_sound/SDL_sound.h
perl -w -pi -e 's/(FILEVERSION 3,0,)\d+/${1}'$NEWVERSION'/;' src/version.rc
perl -w -pi -e 's/(PRODUCTVERSION 3,0,)\d+/${1}'$NEWVERSION'/;' src/version.rc
perl -w -pi -e 's/(VALUE "FileVersion", "3, 0, )\d+/${1}'$NEWVERSION'/;' src/version.rc
perl -w -pi -e 's/(VALUE "ProductVersion", "3, 0, )\d+/${1}'$NEWVERSION'/;' src/version.rc

echo "All done."
echo "Run 'git diff' and make sure this looks correct before 'git commit'."

exit 0

