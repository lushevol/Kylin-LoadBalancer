obj/Release/klbmanager/xml/tinyxml.o obj/Release/klbmanager/xml/tinyxml.d: src/xml/tinyxml.cpp src/xml/tinyxml.h
	rm -f obj/Release/klbmanager/xml/tinyxml.d
	make -f make.do obj/Release/klbmanager/xml/tinyxml.d -e CC:='g++' CPPFLAGS:='-Wall -fexceptions -I/usr/src/linux/include -fpermissive -O3' INCLUDES:='' DEFINE:='-DTIXML_USE_STL -D__EXPORTED_HEADERS__ -D__SERVER__ -D__RELEASE__ -DNDEBUG'
	g++ -Wall -fexceptions -I/usr/src/linux/include -fpermissive -O3  -DTIXML_USE_STL -D__EXPORTED_HEADERS__ -D__SERVER__ -D__RELEASE__ -DNDEBUG -c -o obj/Release/klbmanager/xml/tinyxml.o src/xml/tinyxml.cpp
%.h: fail
	
.PHONY:fail
fail:
	
