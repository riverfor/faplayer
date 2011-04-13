#!/usr/bin/env ruby

all = Array.new
list = `find . -name Android.mk`.split("\n")
list.each { |l|
    next if ((l =~ /^\.\/jni\/vlc\/modules/) == nil)
    temp = l.scan(/modules\/([^\/]+)\//)
    next if temp == nil || temp.size == 0
    File.open(l) { |f|
        while !f.eof?
            ln = f.readline
            next if ((ln =~ /^LOCAL_MODULE/) == nil)
            temp = ln.scan(/\s*LOCAL_MODULE\s*:=\s*([^\s]+)/)
            next if temp == nil || temp.size == 0
            name = temp[0][0].to_s
            name = name[3..-1] if (name =~ /^lib/) != nil
            all.push(name)
        end
    }
}
all.sort!
f = File.open('jni/vlc/src/libvlcjni.h', 'w') { |f|
    f.write("/* auto generated */\n")
    all.each { |m|
        a = m.sub(/_plugin$/, '')
        f.write("vlc_declare_plugin(#{a});\n")
    }
    f.write("const void *vlc_builtins_modules[] = {\n");
    all.each { |m|
        a = m.sub(/_plugin$/, '')
        f.write("\tvlc_plugin(#{a}),\n");
    }
    f.write("\tNULL\n");
    f.write("};\n");
    f.write("/* auto generated */\n")
}
n = `grep -n '# modules' jni/vlc/Android.mk | cut -d: -f1`
n = n.to_i + 1
all = all.join(' ')
`sed -i "#{n} c\\LOCAL_STATIC_LIBRARIES += #{all}" jni/vlc/Android.mk`

