#!/usr/bin/env ruby

require 'pathname'

my_path = Pathname.new(File.dirname(__FILE__)).realpath.to_s

abi = `cat #{my_path}/jni/Application.mk | grep APP_ABI | awk '{ print $3 }'`
abi.strip!
feature =`cat #{my_path}/jni/Application.mk | grep -P 'BUILD_WITH_([^_]+?)_([^_]+?)' | awk '{ print $1 }' | awk -F_ '{ print $4 }'`
feature.strip!
feature.downcase!

all = Array.new
list = `find #{my_path}/jni -name Android.mk`.split("\n")
list.each { |l|
    next if ((l =~ /\/jni\/vlc\/modules/) == nil)
    temp = l.scan(/modules\/([^\/]+)\//)
    next if temp == nil || temp.size == 0
    File.open(l) { |f|
        req_abi = nil
        req_feature = nil
        while !f.eof?
            ln = f.readline
            if (ln =~ /^#\@/)
                temp = ln.scan(/^#\@\s*([^\s]+)\s*([^\s]+)/)
                req_abi = temp[0][0]
                req_feature = temp[0][1]
            end
            next if ((ln =~ /^LOCAL_MODULE/) == nil)
            temp = ln.scan(/\s*LOCAL_MODULE\s*:=\s*([^\s]+)/)
            next if temp == nil || temp.size == 0
            name = temp[0][0].to_s
            name = name[3..-1] if (name =~ /^lib/) != nil
            if (req_abi == nil && req_feature == nil) || (req_abi == abi && req_feature == feature)
                all.push(name)
            end
            req_abi = nil
            req_feature = nil
        end
    }
}
all.sort!
jni_h_old = `cat #{my_path}/jni/vlc/src/libvlcjni.h`
jni_h_new = ''
jni_h_new += "/* auto generated */\n"
all.each { |m|
    a = m.sub(/_plugin$/, '')
    jni_h_new += "vlc_declare_plugin(#{a});\n"
}
jni_h_new += "const void *vlc_builtins_modules[] = {\n"
all.each { |m|
    a = m.sub(/_plugin$/, '')
    jni_h_new += "\tvlc_plugin(#{a}),\n"
}
jni_h_new += "\tNULL\n"
jni_h_new += "};\n"
jni_h_new += "/* auto generated */\n"
if jni_h_old != jni_h_new
    f = File.open("#{my_path}/jni/vlc/src/libvlcjni.h", 'w') { |f|
        f.write(jni_h_new)
    }
end
n = `grep -n '# modules' #{my_path}/jni/vlc/Modules.mk | cut -d: -f1`
n = n.to_i + 1
modules_old = `sed -n #{n}p #{my_path}/jni/vlc/Modules.mk`.strip!
modules_new = 'LOCAL_STATIC_LIBRARIES += ' + all.join(' ')
if modules_old != modules_new
    `sed -i "#{n} c\\#{modules_new}" #{my_path}/jni/vlc/Modules.mk`
end

