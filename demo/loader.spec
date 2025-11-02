name     "Simple Loader (Hooking)"
describe "Simple DLL loader that masks DLL content when MessageBoxA called"
author   "Raphael Mudge"

x64:
	load "bin/loader.x64.o"
		make pic +gofirst

		dfr "resolve" "ror13"
		mergelib "../libtcg/libtcg.x64.zip"

		push $DLL
			link "my_data"

		load "bin/free.x64.o"
			make object
			mergelib "../libipc/libipc.x64.zip"
			export
			link "my_bof"

		#generate $HKEY 128

		load "bin/hook.x64.o"
			make object
			#patch "xorkey" $HKEY
			mergelib "../libipc/libipc.x64.zip"
			export
			link "my_hooks"

		export
