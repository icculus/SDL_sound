#!/bin/sh

if [ -z $1 ]; then
    echo "USAGE: $0 <new_patch_version>" 1>&2
    exit 1
fi

NEWVERSION="$1"
echo "Updating version to '3.2.$NEWVERSION' ..."

perl -w -pi -e 's/(set\(MICRO_VERSION )\d+/${1}'$NEWVERSION'/;' CMakeLists.txt
perl -w -pi -e 's/(\#define SDL_SOUND_MICRO_VERSION )\d+/${1}'$NEWVERSION'/;' include/SDL3_sound/SDL_sound.h
perl -w -pi -e 's/(FILEVERSION 3,2,)\d+/${1}'$NEWVERSION'/;' src/version.rc
perl -w -pi -e 's/(PRODUCTVERSION 3,2,)\d+/${1}'$NEWVERSION'/;' src/version.rc
perl -w -pi -e 's/(VALUE "FileVersion", "3, 2, )\d+/${1}'$NEWVERSION'/;' src/version.rc
perl -w -pi -e 's/(VALUE "ProductVersion", "3, 2, )\d+/${1}'$NEWVERSION'/;' src/version.rc
perl -w -pi -e 's/(\#define PLAYSOUND_VER_PATCH\s+)\d+/${1}'$NEWVERSION'/;' examples/playsound.c

echo "All done."
echo "Run 'git diff' and make sure this looks correct before 'git commit'."

exit 0

