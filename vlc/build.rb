#!/usr/bin/env ruby

require 'open3'
require 'pathname'

# TODO: too ugly...

my_path = Pathname.new(File.dirname(__FILE__)).realpath.to_s

# full path to NDK root
ndk_r6b = '/opt/android/android-ndk-r6b'
ndk_mips = ''

# package name prefix
package_name_prefix = 'org.stagex.danmaku.player.vlc'

# common optimization cflags
common_cflags = '-O3 -fstrict-aliasing -fprefetch-loop-arrays -ffast-math'

# for different arch
arch_specific_cflags = {
    'arm' => '-mlong-calls',
    'x86' => '',
    'mips' => '',
}

# 
config = [
{
    :arch => 'arm',
    :abi => 'armeabi-v7a',
    :feature => 'neon',
    :cflags => '-march=armv7-a -mtune=cortex-a8 -mfpu=neon -mvectorize-with-neon-quad',
    :ndk => ndk_r6b,
},
{
    :arch => 'arm',
    :abi => 'armeabi-v7a',
    :feature => 'vfpv3-d16',
    :cflags => '-march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3-d16',
    :ndk => ndk_r6b,
},
{
    :arch => 'arm',
    :abi => 'armeabi',
    :feature => 'vfp',
    :cflags => '-march=armv6j -mtune=arm1136jf-s -mfpu=vfp',
    :ndk => ndk_r6b,
},
{
    :arch => 'arm',
    :abi => 'armeabi',
    :feature => '',
    :cflags => '-march=armv6j -mtune=arm1136j-s -msoft-float',
    :ndk => ndk_r6b,
},
]

config.each { |c|
    abi = c[:abi].gsub(/[^0-9a-z]/, '')
    feature = c[:feature].gsub(/[^0-9a-z]/, '')
    package = "#{package_name_prefix}.#{abi}"
    application_name = "VlcJni-#{c[:arch].upcase}"
    if feature.length > 0
        application_name = "#{application_name}-#{feature.upcase}"
        package = "#{package}.#{feature}"
    end
    now = Time.new
    version_code = now.to_i
    version_name = now.strftime("%Y.%m.%d.%H.%M.%S")
    androidmanifest_xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>
<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"
      package=\"#{package}\"
      android:versionCode=\"#{version_code}\"
      android:versionName=\"#{version_name}\">
    <application android:name=\"#{application_name}\" android:icon=\"@drawable/icon\" android:label=\"@string/name\">
    </application>
    <uses-sdk android:minSdkVersion=\"3\" />
</manifest>"
    File.open(my_path + '/AndroidManifest.xml', File::CREAT|File::TRUNC|File::RDWR, 0644) { |f|
        f.write(androidmanifest_xml)
    }
    application_mk = "OPT_CFLAGS := #{common_cflags} #{arch_specific_cflags[c[:arch]]} #{c[:cflags]}
APP_OPTIM := release
APP_ABI := #{c[:abi]}
APP_CFLAGS := $(APP_CFLAGS) $(OPT_CFLAGS)
APP_CPPFLAGS := $(APP_CPPFLAGS) $(OPT_CPPFLAGS) 
APP_STL := gnustl_static"
    application_mk += "\nBUILD_WITH_#{c[:arch].upcase} := 1"
    application_mk += "\nBUILD_WITH_#{c[:arch].upcase}_#{feature.upcase} := 1" if c[:feature].length > 0
    File.open(my_path + '/jni/Application.mk', File::CREAT|File::TRUNC|File::RDWR, 0644) { |f|
        f.write(application_mk)
    }
    fn = "#{c[:arch].upcase}"
    fn = "#{fn}-#{feature.upcase}" if feature.length > 0
    print "building faplayer-jni-#{fn}.apk\n"
    `#{my_path}/pre-build.rb`
    `cd #{my_path} && #{c[:ndk]}/ndk-build -j4 2>&1 >> ndk-build.log`
    if !File.exists?("#{my_path}/libs/#{c[:abi]}/libvlccore.so")
        print "build failed for #{c[:abi]} #{c[:feature]}\n"
        exit 1
    end
    `#{my_path}/post-build.rb`
    `cd #{my_path} && ant release`
    `java -jar #{my_path}/../signapk.jar #{my_path}/../testkey.x509.pem #{my_path}/../testkey.pk8 #{my_path}/bin/faplayer-jni-unsigned.apk #{my_path}/bin/faplayer-jni-#{fn}.apk`
    print "built faplayer-jni-#{fn}.apk\n"
}

