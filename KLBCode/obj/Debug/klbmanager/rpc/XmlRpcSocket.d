obj/Debug/klbmanager/rpc/XmlRpcSocket.o obj/Debug/klbmanager/rpc/XmlRpcSocket.d: src/rpc/XmlRpcSocket.cpp src/rpc/XmlRpcSocket.h \
 src/rpc/XmlRpcUtil.h
	rm -f obj/Debug/klbmanager/rpc/XmlRpcSocket.d
	make -f make.do obj/Debug/klbmanager/rpc/XmlRpcSocket.d -e CC:='g++' CPPFLAGS:='-Wall -fexceptions -I/usr/src/linux/include -fpermissive -g' INCLUDES:='' DEFINE:='-DTIXML_USE_STL -D__EXPORTED_HEADERS__ -D__SERVER__ -D__DEBUG__'
	g++ -Wall -fexceptions -I/usr/src/linux/include -fpermissive -g  -DTIXML_USE_STL -D__EXPORTED_HEADERS__ -D__SERVER__ -D__DEBUG__ -c -o obj/Debug/klbmanager/rpc/XmlRpcSocket.o src/rpc/XmlRpcSocket.cpp
%.h: fail
	
.PHONY:fail
fail:
	