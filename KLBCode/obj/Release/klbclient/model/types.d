obj/Release/klbclient/model/types.o obj/Release/klbclient/model/types.d: src/model/types.cpp src/model/share/include.h src/model/types.h \
 src/model/model.h src/model/rpc/XmlRpc.h src/model/rpc/XmlRpcClient.h \
 src/model/rpc/XmlRpcDispatch.h src/model/rpc/XmlRpcSource.h \
 src/model/rpc/XmlRpcException.h src/model/rpc/XmlRpcServer.h \
 src/model/rpc/XmlRpcServerMethod.h src/model/rpc/XmlRpcValue.h \
 src/model/rpc/xml/tinyxml.h src/model/rpc/XmlRpcUtil.h \
 src/model/exception.h
	rm -f obj/Release/klbclient/model/types.d
	make -f make.do obj/Release/klbclient/model/types.d -e CC:='g++' CPPFLAGS:='-Wall -fexceptions -I/usr/src/linux/include -fpermissive -O3' INCLUDES:='' DEFINE:='-DTIXML_USE_STL -D__EXPORTED_HEADERS__ -D__CLIENT__ -D__RELEASE__ -DNDEBUG'
	g++ -Wall -fexceptions -I/usr/src/linux/include -fpermissive -O3  -DTIXML_USE_STL -D__EXPORTED_HEADERS__ -D__CLIENT__ -D__RELEASE__ -DNDEBUG -c -o obj/Release/klbclient/model/types.o src/model/types.cpp
%.h: fail
	
.PHONY:fail
fail:
	
