#/bin/zsh
# default shell on MacOD

# -- make_distribution.sh --

if [[ $# == 0 ]]; then
	echo "Usage: "$0" <path to BeebEm5.app>"
	exit
fi

DIST_FOLDER=./beebem5-distribution

APP_BUNDLE="$1"


if [[ -d "$APP_BUNDLE" ]]; then
	APP_BUNDLE="$APP_BUNDLE/BeebEm5.app"
fi

echo Copying from "$APP_BUNDLE" to "$DIST_FOLDER"

# copy the bundle to the distribution
rm -rf "$DIST_FOLDER/BeebEm5.app"
cp -r "$APP_BUNDLE" "$DIST_FOLDER" || exit

# clear the extended attributes
xattr -cr "$DIST_FOLDER"

# recreate the distribution zip file
rm beebem5-distribution.zip
zip beebem5-distribution -r "$DIST_FOLDER" -x "*.DS_Store" -x "__MACOSX"

