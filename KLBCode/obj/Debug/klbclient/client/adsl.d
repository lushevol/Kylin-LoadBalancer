obj/Debug/klbclient/client/adsl.o obj/Debug/klbclient/client/adsl.d: src/client/adsl.cpp src/client/model/adsl.h \
 src/client/model/model.h src/client/model/share/include.h \
 src/client/model/share/debug.h src/client/model/rpc/XmlRpc.h \
 src/client/model/rpc/XmlRpcClient.h \
 src/client/model/rpc/XmlRpcDispatch.h \
 src/client/model/rpc/XmlRpcSource.h \
 src/client/model/rpc/XmlRpcException.h \
 src/client/model/rpc/XmlRpcServer.h \
 src/client/model/rpc/XmlRpcServerMethod.h \
 src/client/model/rpc/XmlRpcValue.h src/client/model/rpc/xml/tinyxml.h \
 src/client/model/rpc/XmlRpcUtil.h src/client/model/exception.h \
 src/client/model/types.h src/client/model/interface.h \
 src/client/control.h src/client/model/model.h src/client/parser.h \
 src/client/tools.h src/client/reader.h
	rm -f obj/Debug/klbclient/client/adsl.d
	make -f make.do obj/Debug/klbclient/client/adsl.d -e CC:='g++' CPPFLAGS:='-Wall -fexceptions -I/usr/src/linux/include -fpermissive -g' INCLUDES:='' DEFINE:='-DTIXML_USE_STL -D__EXPORTED_HEADERS__ -D__CLIENT__ -D__DEBUG__'
	g++ -Wall -fexceptions -I/usr/src/linux/include -fpermissive -g  -DTIXML_USE_STL -D__EXPORTED_HEADERS__ -D__CLIENT__ -D__DEBUG__ -c -o obj/Debug/klbclient/client/adsl.o src/client/adsl.cpp
%.h: fail
	
.PHONY:fail
fail:
	
