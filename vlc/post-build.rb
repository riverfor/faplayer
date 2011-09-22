#!/usr/bin/env ruby

`find libs -name libbinder.so | xargs rm`
`find libs -name libcutils.so | xargs rm`
`find libs -name libmedia.so | xargs rm`
`find libs -name libstagefright.so | xargs rm`
`find libs -name libutils.so | xargs rm`

