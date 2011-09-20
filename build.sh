#!/bin/sh

ruby vlc/build.rb || exit 1
# XXX
rm -rf lib
unzip -o vlc/bin/faplayer-jni-ARM-NEON.apk 'lib/*'
mv lib libs
ant release
test -f bin/faplayer-unsigned.apk || exit 1
java -jar signapk.jar testkey.x509.pem testkey.pk8 bin/faplayer-unsigned.apk bin/faplayer.apk
mv bin/faplayer.apk bin/faplayer-not-aligned.apk
zipalign -f 4 bin/faplayer-not-aligned.apk bin/faplayer.apk
echo "Done"

