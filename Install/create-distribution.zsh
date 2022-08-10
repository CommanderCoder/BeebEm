#!/bin/zsh

if [[ $# == 0 ]]; then
	echo "Usage: "$0" <path to BeebEm5.app>"
	exit
fi

# $SW_FOLDER contains all the files to go along with the APP, i.e. example disks and tapes
SW_FOLDER=software
APP_BUNDLE="$1"
DIST_FOLDER=BeebEm5-files


if [[ -d "$APP_BUNDLE" ]]; then
	APP_BUNDLE="$APP_BUNDLE/BeebEm5.app"
fi

# did we get it?
test -e $APP_BUNDLE || exit

echo Copying from "$APP_BUNDLE" to "$DIST_FOLDER"

# copy the bundle to the distribution
mkdir "$DIST_FOLDER"
cp -af "$APP_BUNDLE" "$DIST_FOLDER" || exit
cp -af "$SW_FOLDER" "$DIST_FOLDER" || exit

# clear the extended attributes
xattr -cr "$DIST_FOLDER"

# recreate the distribution zip file
[[ -f BeebEm5.zip ]] && rm BeebEm5.zip
zip BeebEm5 -r "$DIST_FOLDER" -x "*.DS_Store" -x "__MACOSX"
rm -rf $DIST_FOLDER

# DMG has links to the Application folder, and the READMEs, COPYRIGHT and LICENCE
# create the DMG
# Since create-dmg does not clobber, be sure to delete previous DMG
[[ -f BeebEm5.dmg ]] && rm BeebEm5.dmg
./create-dmg/create-dmg \
     --background background.png \
     --window-size 620 430 \
     --icon BeebEm5.app 100 190 \
     --app-drop-link 250 190 \
     --add-folder $SW_FOLDER $SW_FOLDER 500 190 \
     BeebEm5.dmg \
     "$APP_BUNDLE"
