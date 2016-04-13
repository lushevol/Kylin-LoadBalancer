#include <sstream>
#include <fstream>

#include "share/include.h"
#include "share/utility.h"

#include "rpc.h"
#include "serialize.h"
#include "base.h"
#include "logger.h"

#include "hvs.h"
#include "services.h"
#include "network.h"
#include "ethernet.h"
#include "bonding.h"
#include "ha.h"

using namespace std;

HttpControl::HttpControl()
    : Http(Configure::GetValue()["Http"])
{}

static const char *HvsConfig = "/etc/nginx/nginx.conf";
static bool NginxRunning = false;

void HttpControl::UpdateNginx()
{
    ofstream hvs(HvsConfig);
    hvs << "user root;" << endl;
    hvs << "error_log /dev/null;" << endl;
    hvs << "worker_processes " << Http.Processor << ";" << endl;
    hvs << "events {" << endl;
    hvs << "\tuse epoll;" << endl;
    hvs << "\tworker_connections " << Http.Connections << ";" << endl;
    hvs << "}\n" << endl;
    hvs << "http {" << endl;
    hvs << "include /etc/nginx/mime.types;" << endl;
    hvs << "default_type application/octet-stream;" << endl;
    hvs << "sendfile on;" << endl;
    hvs << "keepalive_timeout " << Http.Keepalive << ";" << endl;
    hvs << "proxy_cache_path /var/klbcache levels=1:2 keys_zone=cache_klb:500m inactive=" << Http.CacheInactive << "m max_size=100g;" << endl;

    if(Http.Gzip)
    {
        hvs << "gzip on;" << endl;
        hvs << "gzip_min_length " << Http.GzipLength << ";" << endl;
        hvs << "gzip_buffers 4 8k;" << endl;
        hvs << "gzip_http_version 1.1;" << endl;
        hvs << "gzip_types text/text text/plain text/xml text/css application/x-javascript application/javascript;" << endl;
        hvs << "tcp_nodelay on;" << endl;
        hvs << "gzip_disable \"MSIE [1-6]\\.\";" << endl;
    }

    ENUM_LIST(GroupItem, Http.GroupList, e)
    {
        GroupItem group(*e);
        if(group.ServerList.IsEmpty())
            continue;
        hvs << "upstream " << group.Name << "{" << endl;
        ENUM_LIST(ServerItem, group.ServerList, p)
        {
            ServerItem server(*p);
            hvs << "\tserver " << server.IP << ":" << server.Port;
            if(server.Type == TypeState::Down)
                hvs << " down;" << endl;
            else if(server.Type == TypeState::Backup)
                hvs << " backup;" << endl;
            else
            {
                hvs << " weight=" << server.Weight << " max_fails=" << server.MaxFails << " fail_timeout=" << server.FailTimeout;
                if((group.Method == MethodState::CookieTomcat || group.Method == MethodState::CookieResin) && server.SrunID != "")
                    hvs << " srun_id=" << server.SrunID << ";" << endl;
                else
                    hvs << ";" << endl;
            }
        }
        switch(group.Method)
        {
            case MethodState::IPHash:
                hvs << "ip_hash;";
                break;
            case MethodState::ProxyIPHash:
                hvs << "hash $http_x_forwarded_for;";
                break;
            case MethodState::Fair:
                hvs << "fair;";
                break;
            case MethodState::URLHash:
                hvs << "hash $request_url;";
                break;
            case MethodState::Cookie:
                hvs << "hash $uid_got;";
                break;
            case MethodState::CookieTomcat:
                hvs << "jvm_route $cookie_JSESSIONID reverse;";
                break;
            case MethodState::CookieResin:
                hvs << "jvm_route $cookie_JSESSIONID;";
                break;
        }
        hvs << endl;
        hvs << "}" << endl;
    }

    if(Http.DenyNotMatch)
    {
        ENUM_LIST(ServiceItem, Http.ServiceList, e)
        {
            hvs << "server {" << endl;
            hvs << "\taccess_log off;" << endl;
            hvs << "\tlisten " << e->IP << ":" << e->Port << ";" << endl;
            hvs << "\tdeny all;" << endl;
            hvs << "}" << endl;
        }
    }

    ENUM_LIST(ServiceItem, Http.ServiceList, e)
    {
        ServiceItem service(*e);
        hvs << "server {" << endl;
        hvs << "\taccess_log off;" << endl;
        hvs << "\tlisten " << service.IP << ":" << service.Port << ";" << endl;
        istringstream stream(service.Name);
        String temp;
        while(stream >> temp)
        {
            hvs << "\tserver_name " << temp << ";" << endl;
            hvs << "\tserver_name *." << temp << ";" << endl;
        }
        if(service.Ssl && ((const BinaryData&)service.Cert).size() > 0 && ((const BinaryData&)service.Key).size() > 0)
        {
            hvs << "\tssl on;" << endl;
            ostringstream keyname;
            keyname << "/etc/nginx/ssl/" << service.ID;
            hvs << "\tssl_certificate /etc/nginx/ssl/" << service.ID << ".cert;" << endl;
            hvs << "\tssl_certificate_key /etc/nginx/ssl/" << service.ID << ".key;" << endl;
            {
                ofstream stream((keyname.str() + ".cert").c_str());
                String data;
                service.Cert.ToString(data);
                stream << data;
            }
            {
                ofstream stream((keyname.str() + ".key").c_str());
                String data;
                service.Key.ToString(data);
                stream << data;
            }
            hvs << "\tssl_session_timeout " << service.SslTimeout << ";" << endl;
            hvs << "\tssl_protocols  SSLv2 SSLv3 TLSv1;" << endl;
        }
        if(service.CookieEnabled && service.CookieName != "")
        {
            hvs << "\tuserid on;" << endl;
            hvs << "\tuserid_name " << service.CookieName << ";" << endl;
            hvs << "\tuserid_expires " << service.CookieExpire << "d;" << endl;
        }
        if(service.LocationList.IsEmpty())
        {
            hvs << "\tdeny all;" << endl;
        } else {
            ENUM_LIST(LocationItem, service.LocationList, p)
            {
                LocationItem location(*p);
                hvs << "\n\tlocation " << location.Match << " {" << endl;
                hvs << "\t\tproxy_pass http://" << location.Group << ";" << endl;
                hvs << "\t\tproxy_set_header Host $host;" << endl;
                hvs << "\t\tproxy_set_header X-Real-IP $remote_addr;" << endl;
                hvs << "\t\tproxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;" << endl;
		if(location.CacheValid)
		{
		    hvs << "\t\tproxy_cache cache_klb;" << endl;
		    hvs << "\t\tproxy_cache_valid " << location.CacheValid << "m;" << endl;
		}
                if(service.Ssl)
                    hvs << "\t\tproxy_set_header X-Forwarded-Proto https;" << endl;
                hvs << "\t\tproxy_redirect off; \n\t}" << endl;
            }
        }
        hvs << "}\n" << endl;
    }
    hvs << "}" << endl;
    hvs.close();

    if(NginxRunning)
    {
        if(Http.Enabled)
        {
            if(Exec::System("nginx -s reload") != 0)
                Http.Status = false;
            else
                Http.Status = true;
        } else {
            Exec::System("killall nginx");
            Http.Status = false;
            NginxRunning = false;
        }
    } else {
        if(Http.Enabled)
        {
            if(Exec::System("nginx") != 0)
                Http.Status = false;
            else
                Http.Status = true;
            NginxRunning = true;
        } else {
            Exec::System("killall -9 nginx");
        }
    }
}

void HttpControl::Set(HttpItem& item)
{
    if(item.Processor.Valid())
        Http.Processor = item.Processor;
    if(item.Connections.Valid())
        Http.Connections = item.Connections;
    if(item.Keepalive.Valid())
        Http.Keepalive = item.Keepalive;
    if(item.Gzip.Valid())
        Http.Gzip = item.Gzip;
    if(item.GzipLength.Valid())
        Http.GzipLength = item.GzipLength;
    if(item.CacheInactive.Valid())
        Http.CacheInactive = item.CacheInactive;
    if(item.DenyNotMatch.Valid())
        Http.DenyNotMatch = item.DenyNotMatch;
    if(item.Enabled.Valid())
    {
        if(Http.Enabled != item.Enabled)
        {
            if(!item.Enabled)
            {
                ENUM_STL(AddressCounter, FAddressCounter, e)
                {
                    DelIP(e->first.first, e->first.second);
                }
                FAddressCounter.clear();
            } else {
                ENUM_LIST(ServiceItem, Http.ServiceList, e)
                {
                    ServiceItem service(*e);
                    if(service.Status)
                    {
                        if(item.Enabled)
                            AddIPCounter(service.IP, service.Dev);
                    }
                }
            }
        }
        Http.Enabled = item.Enabled;
    }
    UpdateNginx();
}

void HttpControl::Get(HttpItem& item)
{
    item.Enabled = Http.Enabled;
    item.Processor = Http.Processor;
    item.Connections = Http.Connections;
    item.Keepalive = Http.Keepalive;
    item.Gzip = Http.Gzip;
    item.GzipLength = Http.GzipLength;
    item.CacheInactive = Http.CacheInactive;
    item.DenyNotMatch = Http.DenyNotMatch;
    item.Status = Http.Status;
}

void HttpControl::AddIP(const String& ip, const String& dev)
{
    ostringstream stream;
    stream << "ip address add " << ip << "/32 dev " << dev << " label " << dev << ":http";
    Exec::System(stream.str());
    PhysicalInterfaceControl::SendArp(ip, dev);
}

void HttpControl::DelIP(const String& ip, const String& dev)
{
    ostringstream stream;
    stream << "ip address del " << ip << "/32 dev " << dev << " label " << dev << ":http";
    Exec::System(stream.str());
}

void HttpControl::EnsureName(const String& name, int id)
{
    StringCollection collection;
    ENUM_LIST(ServiceItem, Http.ServiceList, e)
    {
        ServiceItem service(*e);
        if(service.ID == id)
            continue;
        istringstream stream(service.Name);
        String temp;
        while(stream >> temp)
            collection.insert(temp);
    }
    istringstream stream(name);
    String temp;
    bool empty = true;
    while(stream >> temp)
    {
        empty = false;
        if(collection.count(temp))
            ERROR(Exception::Http::NameDuplicate, temp);
    }
    if(empty)
        ERROR(Exception::Http::NameEmpty, "");
}

HttpControl::AddressCounter HttpControl::FAddressCounter;

void HttpControl::AddIPCounter(const String& ip, const String& dev)
{
    int& counter = FAddressCounter[AddressPair(ip, dev)];
    if(counter++ == 0)
    {
        AddIP(ip, dev);
    }
}

void HttpControl::DelIPCounter(const String& ip, const String& dev)
{
    AddressPair address(ip, dev);
    AddressCounter::iterator pos = FAddressCounter.find(address);
    if(pos != FAddressCounter.end())
    {
        if(--(pos->second) == 0)
        {
            FAddressCounter.erase(pos);
            DelIP(ip, dev);
        }
    }
}

void HttpControl::ServiceGet(ServiceItem& item)
{
    ServiceItem service(Http.ServiceList.Get(item.ID));
    item.Name = service.Name;
    item.IP = service.IP;
    item.Dev = service.Dev;
    item.Status = service.Status;
    item.Ssl = service.Ssl;
    item.SslTimeout = service.SslTimeout;
    item.Port = service.Port;
    item.LocationList = service.LocationList;
    item.HA = service.HA;
    item.HAStatus = service.HAStatus;
    item.CookieEnabled = service.CookieEnabled;
    item.CookieExpire = service.CookieExpire;
    item.CookieName = service.CookieName;
#ifdef __DEBUG__
    item.Cert = service.Cert;
    item.Key = service.Key;
#endif
}

void HttpControl::ServiceList(List<ServiceItem>& list)
{
    list.Clear();
    ENUM_LIST(ServiceItem, Http.ServiceList, e)
    {
        ServiceItem dst(list.Append());
        ServiceItem src(*e);
        dst.Status = src.Status;
        dst.ID = src.ID;
        dst.Name = src.Name;
        dst.IP = src.IP;
        dst.Dev = src.Dev;
        dst.Ssl = src.Ssl;
        dst.SslTimeout = src.SslTimeout;
        dst.Port = src.Port;
        dst.LocationList = src.LocationList;
        dst.HA = src.HA;
        dst.HAStatus = src.HAStatus;
        dst.CookieEnabled = src.CookieEnabled;
        dst.CookieExpire = src.CookieExpire;
        dst.CookieName = src.CookieName;
    }
}

void HttpControl::ServiceListExport(List<ServiceItem>& list)
{
    list.Clear();
    ENUM_LIST(ServiceItem, Http.ServiceList, e)
    {
        ServiceItem dst(list.Append());
        ServiceItem src(*e);
        dst.ID = src.ID;
        dst.Name = src.Name;
        dst.IP = src.IP;
        dst.Dev = src.Dev;
        dst.Ssl = src.Ssl;
        dst.SslTimeout = src.SslTimeout;
        dst.Port = src.Port;
        dst.Cert = src.Cert;
        dst.Key = src.Key;
        dst.LocationList = src.LocationList;
        dst.HA = src.HA;
        dst.CookieEnabled = src.CookieEnabled;
        dst.CookieExpire = src.CookieExpire;
        dst.CookieName = src.CookieName;
    }
}

bool HttpControl::CheckIP(const String& ip, List<HostPackStringValue>& iplist)
{
    if(iplist.IsEmpty())
        return false;
    uint32_t ipaddr;
    VERIFY(Address::StringToAddress(ip, ipaddr));
    ENUM_LIST(HostPackStringValue, iplist, e)
    {
        HostPackStringValue ippack(*e);
        uint32_t packaddr, maskaddr;
        int mask;
        VERIFY(Address::StringToAddressPack(ippack, packaddr, mask));
        VERIFY(Address::NetmaskToAddress(mask, maskaddr));
        if(ipaddr == packaddr || ((ipaddr & maskaddr) != (packaddr & maskaddr)))
            return false;
    }
    return true;
}

bool HttpControl::CheckAddress(const AddressPair& address)
{
    VirtualServiceControl control;
    ENUM_LIST(VirtualServiceItem, control.Holder, e)
    {
        VirtualServiceItem service(*e);
        ENUM_LIST(VirtualServiceItem::IPItem, service.IP, f)
        {
            VirtualServiceItem::IPItem ip(*f);
            if(ip.IP == address.first && ip.Dev == address.second)
                return false;
        }
    }
    switch(Network::GetDevType(address.second))
    {
        case Network::Ethernet:
            {
                EthernetInterfaceControl control;
                EthernetInterface eth(control.Find(address.second));
                if(eth.Master == "" && eth.Adsl == "" && !eth.Dhcp && eth.Enabled)
                    return CheckIP(address.first, eth.IP);
            }
            break;
        case Network::Bonding:
            {
                BondingInterfaceControl control;
                BondingInterface bond(control.Find(address.second));
                if(!bond.Dhcp && !bond.Slaves.IsEmpty() && bond.Enabled)
                    return CheckIP(address.first, bond.IP);
            }
            break;
        default:
            break;
    };
    return false;
}

bool HttpControl::GetHAStatus(int state)
{
    switch(state)
    {
        case HAState::Local:
            return true;
        case HAState::Self:
            return HAControl().HA.Self;
        case HAState::Other:
            return HAControl().HA.Other;
        default:
            return false;
    }
}

void HttpControl::EnsurePort(int port)
{
    if(port == 8888 || port == 8080)
        ERROR(Exception::Http::PortUsed, port);
}

void HttpControl::ServiceAdd(ServiceItem& item)
{
    EnsureName(item.Name, -1);
    if(item.IP == "" || item.Dev == "")
        ERROR(Exception::Http::AddressEmpty, "");
    item.Port.Valid();
    item.Ssl.Valid();
    item.SslTimeout.Valid();
    item.Cert.Valid();
    item.Key.Valid();
    item.HA.Valid();
    item.CookieEnabled.Valid();
    item.CookieExpire.Valid();
    item.CookieName.Valid();
    EnsurePort(item.Port);
    bool status = CheckAddress(AddressPair(item.IP, item.Dev));
    ServiceItem service(Http.ServiceList.Append());
    service.ID = Http.ServiceList.GetCount() - 1;
    service.Name = item.Name;
    service.IP = item.IP;
    service.Dev = item.Dev;
    service.HA = item.HA;
    service.HAStatus = GetHAStatus(service.HA);
    service.Status = status;
    if(Http.Enabled && service.Status && service.HAStatus)
        AddIPCounter(service.IP, service.Dev);
    service.Ssl = item.Ssl;
    service.SslTimeout = item.SslTimeout;
    service.Port = item.Port;
    service.Cert = item.Cert;
    service.Key = item.Key;
    service.CookieEnabled = item.CookieEnabled;
    service.CookieExpire = item.CookieExpire;
    service.CookieName = item.CookieName;
    service.LocationList.Clear();
    UpdateNginx();
}

void HttpControl::ServiceDel(ServiceItem& item)
{
    ServiceItem service(Http.ServiceList.Get(item.ID));
    if(Http.Enabled && service.Status && service.HAStatus)
        DelIPCounter(service.IP, service.Dev);
    Http.ServiceList.Delete(item.ID);
    int count = 0;
    ENUM_LIST(ServiceItem, Http.ServiceList, e)
    {
        e->ID = count++;
    }
    UpdateNginx();
}

void HttpControl::ServiceSet(ServiceItem& item)
{
    ServiceItem service(Http.ServiceList.Get(item.ID));
    String name = item.Name.Valid() ? item.Name : service.Name;
    String ip = item.IP.Valid() ? item.IP : service.IP;
    String dev = item.Dev.Valid() ? item.Dev : service.Dev;
    int ha = item.HA.Valid() ? item.HA : service.HA;
    int port = item.Port.Valid() ? item.Port : service.Port;
    EnsurePort(port);
    EnsureName(name, item.ID);
    if(ip == "" || dev == "")
        ERROR(Exception::Http::AddressEmpty, "");
    bool status = CheckAddress(AddressPair(ip, dev));
    bool hastatus = GetHAStatus(ha);
    if(Http.Enabled && (service.Status != status || service.IP != ip || service.Dev != dev || hastatus != service.HAStatus))
    {
        if(service.Status && service.HAStatus)
            DelIPCounter(service.IP, service.Dev);
        if(status && hastatus)
            AddIPCounter(ip, dev);
    }
    service.HAStatus = hastatus;
    service.Status = status;
    service.Name = name;
    service.IP = ip;
    service.Dev = dev;
    service.Port = port;
    if(item.Ssl.Valid())
        service.Ssl = item.Ssl;
    if(item.SslTimeout.Valid())
        service.SslTimeout = item.SslTimeout;
    if(item.Cert.Valid())
        service.Cert = item.Cert;
    if(item.Key.Valid())
        service.Key = item.Key;
    if(item.CookieEnabled.Valid())
        service.CookieEnabled = item.CookieEnabled;
    if(item.CookieExpire.Valid())
        service.CookieExpire = item.CookieExpire;
    if(item.CookieName.Valid())
        service.CookieName = item.CookieName;
    UpdateNginx();
}

void HttpControl::CheckGroup(const String& group)
{
    ENUM_LIST(GroupItem, Http.GroupList, e)
    {
        if(e->Name == group)
            return;
    }
    ERROR(Exception::Http::LocationGroupNotFound, group);
}

void HttpControl::LocationAdd(LocationItem& item)
{
    ServiceItem service(Http.ServiceList.Get(item.ServiceID));
    if(item.Match == "")
        ERROR(Exception::Http::LocationMatchEmpty, "");
    CheckGroup(item.Group);
    if(item.ID.Valid())
    {
        service.LocationList.Insert(item.ID);
        for(int i = item.ID + 1; i < service.LocationList.GetCount(); ++i)
        {
            LocationItem(service.LocationList.Get(i)).ID = i;
        }
    } else {
        service.LocationList.Append();
        item.ID = service.LocationList.GetCount() - 1;
    }
    LocationItem location(service.LocationList.Get(item.ID));
    location.ID = item.ID;
    location.Match = item.Match;
    location.Group = item.Group;
    location.CacheValid = item.CacheValid;
    UpdateNginx();
}

void HttpControl::LocationDel(LocationItem& item)
{
    ServiceItem service(Http.ServiceList.Get(item.ServiceID));
    service.LocationList.Get(item.ID);
    service.LocationList.Delete(item.ID);
    int count = 0;
    ENUM_LIST(LocationItem, service.LocationList, e)
    {
        e->ID = count++;
    }
    UpdateNginx();
}

void HttpControl::LocationSet(LocationItem& item)
{
    ServiceItem service(Http.ServiceList.Get(item.ServiceID));
    LocationItem location(service.LocationList.Get(item.ID));
    if(item.Match.Valid() && item.Match == "")
        ERROR(Exception::Http::LocationMatchEmpty, "");
    if(item.Group.Valid())
        CheckGroup(item.Group);
    if(item.Match.Valid())
        location.Match = item.Match;
    if(item.Group.Valid())
        location.Group = item.Group;
    if(item.CacheValid.Valid())
        location.CacheValid = item.CacheValid;
    UpdateNginx();
}

void HttpControl::EnsureGroup(const String& group, int id)
{
    if(group == "")
        ERROR(Exception::Http::GroupNameEmpty, "");
    ENUM_LIST(GroupItem, Http.GroupList, e)
    {
        GroupItem groupitem(*e);
        if(groupitem.ID == id)
            continue;
        if(groupitem.Name == group)
            ERROR(Exception::Http::GroupNameDuplicate, group);
    }
}

void HttpControl::GroupGet(GroupItem& item)
{
    GroupItem group(Http.GroupList.Get(item.ID));
    item.Name = group.Name;
    item.Method = group.Method;
    item.ServerList = group.ServerList;
}

void HttpControl::GroupList(List<GroupItem>& list)
{
    list.Clear();
    ENUM_LIST(GroupItem, Http.GroupList, e)
    {
        GroupItem dst(list.Append());
        GroupItem src(*e);
        dst.ID = src.ID;
        dst.Name = src.Name;
        dst.Method = src.Method;
        dst.ServerList = src.ServerList;
    }
}

void HttpControl::GroupAdd(GroupItem& item)
{
    EnsureGroup(item.Name, -1);
    GroupItem group(Http.GroupList.Append());
    group.ID = Http.GroupList.GetCount() - 1;
    group.Name = item.Name;
    group.Method = item.Method;
    group.ServerList.Clear();
}

void HttpControl::GroupDel(GroupItem& item)
{
    GroupItem group(Http.GroupList.Get(item.ID));
    ENUM_LIST(ServiceItem, Http.ServiceList, service)
    {
        ENUM_LIST(LocationItem, service->LocationList, location)
        {
            if(location->Group == group.Name)
                ERROR(Exception::Http::GroupUsed, service->Name);
        }
    }
    Http.GroupList.Delete(item.ID);
    int count = 0;
    ENUM_LIST(GroupItem, Http.GroupList, e)
    {
        e->ID = count++;
    }
    UpdateNginx();
}

void HttpControl::GroupSet(GroupItem& item)
{
    GroupItem group(Http.GroupList.Get(item.ID));
    String name = item.Name.Valid() ? item.Name : group.Name;
    EnsureGroup(name, item.ID);
    if(item.Method.Valid())
        group.Method = item.Method;
    if(group.Name != name)
    {
        ENUM_LIST(ServiceItem, Http.ServiceList, service)
        {
            ENUM_LIST(LocationItem, service->LocationList, location)
            {
                if(location->Group == group.Name)
                    location->Group = name;
            }
        }
        group.Name = name;
    }
    UpdateNginx();
}

void HttpControl::ServerAdd(ServerItem& item)
{
    GroupItem group(Http.GroupList.Get(item.GroupID));
    if(item.IP == "")
        ERROR(Exception::Http::ServerIPEmpty, "");
    String ip = item.IP;
    int port =  item.Port;
    int weight =  item.Weight;
    int fail =  item.MaxFails;
    int timeout = item.FailTimeout;
    int type = item.Type;
    String srunid = item.SrunID;
    ServerItem server(group.ServerList.Append());
    server.ID = group.ServerList.GetCount() - 1;
    server.IP = ip;
    server.Port = port;
    server.Weight = weight;
    server.MaxFails = fail;
    server.FailTimeout = timeout;
    server.Type = type;
    server.SrunID = srunid;
    UpdateNginx();
}

void HttpControl::ServerDel(ServerItem& item)
{
    GroupItem group(Http.GroupList.Get(item.GroupID));
    group.ServerList.Delete(item.ID);
    int count = 0;
    ENUM_LIST(ServerItem, group.ServerList, e)
    {
        e->ID = count++;
    }
    UpdateNginx();
}

void HttpControl::ServerSet(ServerItem& item)
{
    GroupItem group(Http.GroupList.Get(item.GroupID));
    ServerItem server(group.ServerList.Get(item.ID));
    String ip = item.IP.Valid() ? item.IP : server.IP;
    int port = item.Port.Valid() ? item.Port : server.Port;
    int weight = item.Weight.Valid() ? item.Weight : server.Weight;
    int fail = item.MaxFails.Valid() ? item.MaxFails : server.MaxFails;
    int timeout = item.FailTimeout.Valid() ? item.FailTimeout : server.FailTimeout;
    int type = item.Type.Valid() ? item.Type : server.Type;
    String srunid = item.SrunID.Valid() ? item.SrunID : server.SrunID;
    server.IP = ip;
    server.Port = port;
    server.Weight = weight;
    server.MaxFails = fail;
    server.FailTimeout = timeout;
    server.Type = type;
    server.SrunID = srunid;
    UpdateNginx();
}

void HttpControl::RefreshAll(const StringCollection& devs)
{
    HttpControl control;
    ENUM_STL(AddressCounter, control.FAddressCounter, e)
    {
        if(devs.count(e->first.second))
        {
            DelIP(e->first.first, e->first.second);
            control.FAddressCounter.erase(e);
        }
    }
    ENUM_LIST(HttpItem::ServiceItem, control.Http.ServiceList, e)
    {
        HttpItem::ServiceItem service(*e);
        if(devs.count(service.Dev))
        {
            service.Status = CheckAddress(AddressPair(service.IP, service.Dev));
            if(control.Http.Enabled && service.Status && service.HAStatus)
                AddIPCounter(service.IP, service.Dev);
        }
    }
    control.UpdateNginx();
}

void HttpControl::RefreshHA()
{
    ENUM_LIST(ServiceItem, Http.ServiceList, e)
    {
        ServiceItem service(*e);
        bool hastatus = GetHAStatus(service.HA);
        if(Http.Enabled && service.Status && (hastatus != service.HAStatus))
        {
            if(hastatus)
                AddIPCounter(service.IP, service.Dev);
            else
                DelIPCounter(service.IP, service.Dev);
        }
        service.HAStatus = hastatus;
    }
    UpdateNginx();
}

namespace Http
{
    DECLARE_INTERFACE_REFRESH(HttpControl::RefreshAll, 8);

#define EXECUTE_RPC_SET(methodname,methodfunc,type)\
    void LINE_NAME(methodfunc)(Value& params, Value& result)\
    {\
        RpcMethod::CheckLicence();\
        HttpControl control;\
        type http(params);\
        control.methodfunc(http);\
        (bool&)result = true;\
    }\
    DECLARE_RPC_METHOD(methodname,LINE_NAME(methodfunc),true,true)

    EXECUTE_RPC_SET(FuncHttpSet, Set, HttpItem);
    EXECUTE_RPC_SET(FuncHttpServiceAdd, ServiceAdd, HttpItem::ServiceItem);
    EXECUTE_RPC_SET(FuncHttpServiceDel, ServiceDel, HttpItem::ServiceItem);
    EXECUTE_RPC_SET(FuncHttpServiceSet, ServiceSet, HttpItem::ServiceItem);
    EXECUTE_RPC_SET(FuncHttpLocationAdd, LocationAdd, HttpItem::ServiceItem::LocationItem);
    EXECUTE_RPC_SET(FuncHttpLocationDel, LocationDel, HttpItem::ServiceItem::LocationItem);
    EXECUTE_RPC_SET(FuncHttpLocationSet, LocationSet, HttpItem::ServiceItem::LocationItem);
    EXECUTE_RPC_SET(FuncHttpGroupAdd, GroupAdd, HttpItem::GroupItem);
    EXECUTE_RPC_SET(FuncHttpGroupDel, GroupDel, HttpItem::GroupItem);
    EXECUTE_RPC_SET(FuncHttpGroupSet, GroupSet, HttpItem::GroupItem);
    EXECUTE_RPC_SET(FuncHttpServerAdd, ServerAdd, HttpItem::GroupItem::ServerItem);
    EXECUTE_RPC_SET(FuncHttpServerDel, ServerDel, HttpItem::GroupItem::ServerItem);
    EXECUTE_RPC_SET(FuncHttpServerSet, ServerSet, HttpItem::GroupItem::ServerItem);

#define EXECUTE_RPC_GET(methodname,methodfunc,type)\
    void LINE_NAME(methodfunc)(Value& params, Value& result)\
    {\
        RpcMethod::CheckLicence();\
        HttpControl control;\
        type http(params);\
        control.methodfunc(http);\
        result = params;\
    }\
    DECLARE_RPC_METHOD(methodname,LINE_NAME(methodfunc),true,true)

    EXECUTE_RPC_GET(FuncHttpGet, Get, HttpItem);
    EXECUTE_RPC_GET(FuncHttpServiceGet, ServiceGet, HttpItem::ServiceItem);
    EXECUTE_RPC_GET(FuncHttpGroupGet, GroupGet, HttpItem::GroupItem);

#define EXECUTE_RPC_LIST(methodname,methodfunc,type)\
    void LINE_NAME(methodfunc)(Value& params, Value& result)\
    {\
        RpcMethod::CheckLicence();\
        HttpControl control;\
        type http(result);\
        control.methodfunc(http);\
    }\
    DECLARE_RPC_METHOD(methodname,LINE_NAME(methodfunc),true,true)

    EXECUTE_RPC_LIST(FuncHttpServiceList, ServiceList, List<HttpItem::ServiceItem>);
    EXECUTE_RPC_LIST(FuncHttpGroupList, GroupList, List<HttpItem::GroupItem>);

    void HttpBeforeImport(Value& data, bool reload)
    {
        HttpControl control;
        {
            Value temp;
            HttpItem http(temp);
            http.Enabled = false;
            control.Set(http);
        }
        while(control.Http.ServiceList.GetCount())
        {
            Value temp;
            HttpItem::ServiceItem service(temp);
            service.ID = control.Http.ServiceList.GetCount() - 1;
            control.ServiceDel(service);
        }
        while(control.Http.GroupList.GetCount())
        {
            Value temp;
            HttpItem::GroupItem group(temp);
            group.ID = control.Http.GroupList.GetCount() - 1;
            control.GroupDel(group);
        }
    }

    void HttpImport(Value& data, bool reload)
    {
        HttpItem result(data["Http"]);
        HttpControl control;
        if(reload)
        {
            ENUM_LIST(HttpItem::GroupItem, result.GroupList, e)
            {
                HttpItem::GroupItem group(*e);
                NO_ERROR(control.GroupAdd(group));
                HttpItem::GroupItem real(control.Http.GroupList.Last());
                ENUM_LIST(HttpItem::GroupItem::ServerItem, group.ServerList, f)
                {
                    HttpItem::GroupItem::ServerItem server(*f);
                    server.GroupID = real.ID;
                    NO_ERROR(control.ServerAdd(server));
                }
            }
            ENUM_LIST(HttpItem::ServiceItem, result.ServiceList, e)
            {
                HttpItem::ServiceItem service(*e);
                NO_ERROR(control.ServiceAdd(service));
                HttpItem::ServiceItem real(control.Http.ServiceList.Last());
                ENUM_LIST(HttpItem::ServiceItem::LocationItem, service.LocationList, f)
                {
                    HttpItem::ServiceItem::LocationItem location(*f);
                    location.ServiceID = real.ID;
                    NO_ERROR(control.LocationAdd(location));
                }
            }
            NO_ERROR(control.Set(result));
        } else {
            ENUM_LIST(HttpItem::GroupItem, result.GroupList, e)
            {
                HttpItem::GroupItem group(*e);
                control.GroupAdd(group);
                HttpItem::GroupItem real(control.Http.GroupList.Last());
                ENUM_LIST(HttpItem::GroupItem::ServerItem, group.ServerList, f)
                {
                    HttpItem::GroupItem::ServerItem server(*f);
                    server.GroupID = real.ID;
                    control.ServerAdd(server);
                }
            }
            ENUM_LIST(HttpItem::ServiceItem, result.ServiceList, e)
            {
                HttpItem::ServiceItem service(*e);
                control.ServiceAdd(service);
                HttpItem::ServiceItem real(control.Http.ServiceList.Last());
                ENUM_LIST(HttpItem::ServiceItem::LocationItem, service.LocationList, f)
                {
                    HttpItem::ServiceItem::LocationItem location(*f);
                    location.ServiceID = real.ID;
                    control.LocationAdd(location);
                }
            }
            control.Set(result);
        }
    }

    void HttpExport(Value& data)
    {
        HttpItem result(data["Http"]);
        HttpControl control;
        control.Get(result);
        result.Status.Data.clear();
        control.ServiceListExport(result.ServiceList);
        ENUM_LIST(HttpItem::ServiceItem, result.ServiceList, e)
        {
            e->Status.Data.clear();
        }
        control.GroupList(result.GroupList);
    }

    DECLARE_SERIALIZE(HttpBeforeImport, HttpImport, NULL, HttpExport, 12);

    void InitNginx()
    {
        Exec::System("killall -9 nginx");
    }

    DECLARE_INIT(InitNginx, NULL, 12);
};
