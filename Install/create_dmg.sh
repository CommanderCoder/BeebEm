#!/bin/bash

###############################################################
#
# NOTE: This script is intended to be run manually and assumes
# 		you have already exported a signed application bundle
#		using the Xcode 'Archive' tool. There is probably a
#		far better way to do this but it will do for now.
#
###############################################################

if [[ $# == 0 ]]; then
    echo "Usage: "$0" <path to BeebEm5.app>"
    exit
fi


# $DIST_FOLDER contains all the files to go along with the APP, i.e. example disks and tapes
DIST_FOLDER=./beebem5-distribution

APPBUNDLE="$1/BeebEm5.app"

# Get code signing details from config file
#arr=($(grep "DEVELOPMENT_TEAM_ID" "../Config/CodeSign.xcconfig"))
#DEVELOPMENT_TEAM_ID=${arr[2]}
#
#
#line=$(grep "DEVELOPMENT_TEAM_NAME" "../Config/CodeSign.xcconfig")
#DEVELOPMENT_TEAM_NAME="${line##*= }"



TEMPLATE=Template5.dmg.gz
OUTPATH=./BeebEm5_DMG
OUTPUT=Temp.dmg
FINAL=BeebEm5.dmg
VOLNAME=BeebEm5

echo "Creating install DMG from template"

rm -rf "$OUTPATH"
mkdir "$OUTPATH"

cp $TEMPLATE $OUTPUT.gz
mv $OUTPUT.gz $OUTPATH
gunzip $OUTPATH/$OUTPUT

echo "Mounting DMG"
hdiutil attach $OUTPATH/$OUTPUT

echo "Copying support files"
cp -R beebem5-distribution/* "/Volumes/$VOLNAME/BeebEm5/"

echo "Copying signed application"
#already copied
#cp -R "$APPBUNDLE" "/Volumes/$VOLNAME/BeebEm5/"

#echo "Copying current source"
#cp -R "$SRC_PATH" "/Volumes/$VOLNAME/BeebEm5/"

xattr -cr "/Volumes/$VOLNAME/BeebEm5"

# Now detach (unmount) the DMG ready for signing
hdiutil detach "/Volumes/$VOLNAME"

echo "Compressing final DMG"
hdiutil convert -format UDZO -o "$OUTPATH/$FINAL" "$OUTPATH/$OUTPUT" -imagekey zlib-level=9

# Remove extended attributes before signing
xattr -cr "$OUTPATH/$FINAL"


# Sign DMG
#echo "Signing DMG with team ID: $DEVELOPMENT_TEAM_ID"
#codesign -s "Developer ID Application: $DEVELOPMENT_TEAM_NAME ($DEVELOPMENT_TEAM_ID)" "$OUTPATH/$FINAL"


# Clean up
echo "Cleaning temporary files"
rm "$OUTPATH/$OUTPUT"


echo "CREATED DMG"

