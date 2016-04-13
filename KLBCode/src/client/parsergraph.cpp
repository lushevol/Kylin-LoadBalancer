#include "model/interface.h"
#include "model/adsl.h"
#include "model/hvs.h"

#include "control.h"
#include "parser.h"

void GetEthernetList(StringCollection& dev)
{
    CommandModel cmd;
    cmd.FuncName = FuncEthernetGetAll;
    cmd.Password = true;
    Value result;
    Rpc::Call(cmd, result);
    ENUM_LIST(EthernetInterface, List<EthernetInterface>(result), e)
    {
        EthernetInterface eth(*e);
        dev.insert(eth.Dev);
    }
}

void GetBondingList(StringCollection& dev)
{
    CommandModel cmd;
    cmd.FuncName = FuncBondingGetAll;
    cmd.Password = true;
    Value result;
    Rpc::Call(cmd, result);
    ENUM_LIST(BondingInterface, List<BondingInterface>(result), e)
    {
        BondingInterface bond(*e);
        dev.insert(bond.Dev);
    }
}

void GetAdslList(StringCollection& dev)
{
    CommandModel cmd;
    cmd.FuncName = FuncAdslGetAll;
    cmd.Password = true;
    Value result;
    Rpc::Call(cmd, result);
    ENUM_LIST(AdslItem, List<AdslItem>(result), e)
    {
        AdslItem adsl(*e);
        dev.insert(adsl.Dev);
    }
}

void GetServiceDev(StringCollection& dev)
{
    NO_ERROR(GetEthernetList(dev));
    NO_ERROR(GetBondingList(dev));
}

void GetAllDev(StringCollection& dev)
{
    NO_ERROR(GetEthernetList(dev));
    NO_ERROR(GetBondingList(dev));
    NO_ERROR(GetAdslList(dev));
}

void GetArpDev(StringCollection& dev)
{
    NO_ERROR(GetEthernetList(dev));
    NO_ERROR(GetBondingList(dev));
}

void GetHttpGroup(StringCollection& groups)
{
    CommandModel cmd;
    cmd.FuncName = FuncHttpGroupList;
    cmd.Password = true;
    Value result;
    Rpc::Call(cmd, result);
    ENUM_LIST(HttpItem::GroupItem, List<HttpItem::GroupItem>(result), e)
    {
        groups.insert(e->Name);
    }
}

Parser::Parser()
{
    SetValueRule("Value");
#if 1
    SetRule("Description", "description ~Value");
    SetValueRule("AllDev", GetAllDev);
    SetValueRule("EthDev", GetEthernetList);
    SetValueRule("BondDev", GetBondingList);
    SetValueRule("AdslDev", GetAdslList);
    {   //Interface
        SetRule("Address", "address ~Value");
        SetRule("MTU", "mtu ~Value");
        SetRule("Dhcp", "dhcp on|off");
        SetRule("ArpState", "arp enable|disable|proxy|reply");
        SetRule("SetIP", "?(add|del) ~Value *~Value");
        SetRule("IP", "ip flush|~SetIP");
        SetRule("FaceSet", "up|down|~Address|~MTU|~Dhcp|~ArpState|~Description|~IP");
        {   //Ethernet
            SetRule("Eth", "ethernet ?list|(~EthDev ?list|~EthSet)");
            SetRule("EthSpeed", "1000|100|10");
            SetRule("EthMode", "mode auto|(half ~EthSpeed)|(full ~EthSpeed)");
            SetRule("EthSet", "~FaceSet|~EthMode");
        }
        {   //Bonding
            SetRule("Bond", "bonding ?list|(~BondDev ?list|~BondSet)|(add ~Value)|(del ~BondDev)");
            SetRule("BondSlaves", "slaves flush|(~EthDev *~EthDev)");
            SetRule("BondMode", "mode rr|backup|xor|broadcast|802.3ad|tlb|alb");
            SetRule("BondCheck", "check ~BondCheckModeOrder|~BondCheckIP");
            SetOrderRule("BondCheckModeOrder", "BondCheckMode BondCheckFrequency", 1, NULL);
            SetRule("BondCheckMode", "mode miimon|arp");
            SetRule("BondCheckFrequency", "frequency ~Value");
            SetRule("BondCheckIP", "ip flush|(~Value *~Value)");
            SetRule("BondSet", "~FaceSet|~BondSlaves|~BondMode|~BondCheck");
        }
        SetRule("Interface", "~Eth|~Bond");
    }
    SetRule("RuleProt", "protocol tcp|udp|icmp|~Value");
    SetRule("RulePort", "port ~Value");
    SetRule("RuleAddress", "~RulePort|(~Value ?~RulePort)");
    SetRule("RuleFrom", "from ~RuleAddress");
    SetRule("RuleTo", "to ~RuleAddress");
    {   //Route
        SetRule("RouteSetGate", "~RouteGate *~RouteGate");
        SetRule("RouteGate", "nexthop ~RouteGateParams");
        SetOrderRule("RouteGateParams", "GateWeight RouteGateTarget", 0, "RouteGateTarget");
        SetOrderRule("RouteGateTarget", "GateVia GateDev", 1, NULL);
        SetRule("GateVia", "via auto|~Value");
        SetRule("GateWeight", "weight ~Value");
        SetRule("GateDev", "dev ~AllDev");
        SetRule("Route", "route ~StaticR|~PolicyR|~SmartR");
        SetRule("StaticNet", "~Value ?(metric ~Value)");
        SetRule("StaticIndex", "(id ~Value)|~StaticNet");
        SetRule("StaticR", "static ?list|(add ~StaticNet ~RouteSetGate)|(del ~StaticIndex)|(~StaticIndex ~RouteSetGate|~Description)");
        SetRule("PolicyR", "policy ?list|(add ~RouteSetGate)|(insert ~Value ~RouteSetGate)|(del ~Value)|(~Value ~RouteSetGate|~Description|(rule (add ~PolicyRule)|(del ~Value)))");
        SetRule("PolicyRule", "all|~PolicyRuleOrder");
        SetOrderRule("PolicyRuleOrder", "RuleProt RuleFrom RuleTo", 1, NULL);
        SetRule("CF", "?(frequency ~Value ?(timeout ~Value))|(timeout ~Value)");
        SetRule("Check", "(disable|(ping ~Value ~CF)|(tcp|udp ~Value port ~Value ~CF))");
        SetRule("SmartR", "smart ?list|(del ~Value)|(~Value ~Description|(check ~Check)|~RouteSetGate|(isp ~Value)|noisp)|(add ?(isp ~Value) ~RouteSetGate)");
    }
    {   //Nat
        SetRule("OutDev", "out ~AllDev");
        SetRule("InDev", "in ~AllDev");
        SetOrderRule("NatSrcMatch", "RuleProt RuleFrom RuleTo OutDev", 1, NULL);
        SetOrderRule("NatDestMatch", "RuleProt RuleFrom RuleTo InDev", 1, NULL);
        SetRule("NatSrcAction", "translate (ip ~Value)|masquerade ?(port ~Value)");
        SetRule("NatDestAction", "translate ip ~Value ?(port ~Value)");
        SetRule("NatSet", "~Value ~Description|enable|disable");
        SetRule("NatDel", "del ~Value");
        SetRule("Nat", "nat (src ~NatSrc)|(dest ~NatDest)");
        SetRule("NatOp", "?list|~NatDel|~NatSet");
        SetRule("NatSrc", "~NatOp|((add|(insert|replace ~Value)) match all|~NatSrcMatch exclude|~NatSrcAction)");
        SetRule("NatDest", "~NatOp|((add|(insert|replace ~Value)) match all|~NatDestMatch exclude|~NatDestAction)");
    }
    {   //Arp
        SetValueRule("ArpDevList", GetArpDev);
        SetRule("ArpIP", "ip ~Value");
        SetRule("ArpMac", "mac ~Value");
        SetRule("ArpDev", "dev ~ArpDevList");
        SetOrderRule("ArpSet", "ArpIP ArpMac ArpDev", 3, NULL);
        SetOrderRule("ArpDel", "ArpIP ArpDev", 2, NULL);
        SetRule("Arp", "arp ?(list ?(static|dynamic|all))|(set ~ArpSet)|(del ~ArpDel)");
    }
    {   //Other
        SetRule("Hostname", "hostname (?get)|(set ~Value)");
        SetRule("AntiDos", "antidos (?status)|enable|disable");
        SetRule("DnsServer", "dns (?list)|(set ~Value *~Value)|flush");
        SetRule("NetworkOther", "network ~DnsServer|~AntiDos|~Hostname");
    }
    {   //Adsl
        SetRule("AdslMTU", "mtu ~Value");
        SetRule("AdslTimeout", "timeout ~Value");
        SetRule("AdslUser", "user ~Value");
        SetOrderRule("AdslSet", "AdslMTU AdslTimeout AdslUser", 1, NULL);
        SetRule("Adsl", "adsl ?list|(del ~AdslDev)|(add ~Value ethernet ~EthDev)|(~AdslDev ?list|dial|hangup|password|~AdslSet|~Description)");
    }
    {   //VirtualServices
        {   //Address
            SetValueRule("ServiceDev", GetServiceDev);
            SetRule("ServiceTcpPort", "tcp ~Value|flush");
            SetRule("ServiceUdpPort", "udp ~Value|flush");
            SetRule("ServiceIP", "ip ~Value ?(dev ~ServiceDev)");
            SetRule("ServiceIPList", "ip flush|(~Value ?(dev ~ServiceDev) *~ServiceIP)");
            SetOrderRule("ServiceAddress", "ServiceTcpPort ServiceUdpPort ServiceIPList", 1, NULL);
        }
        {   //ServiceVS
            SetRule("ServiceSchedule", "schedule rr|wrr|lc|wlc|lblc|lblcr|dh|sh|sed|nq");
            SetRule("ServicePersistent", "persistent off|(~Value ?(netmask ~Value))");
            {   //Monitor
                SetRule("ServiceMonitorInterval", "interval ~Value");
                SetRule("ServiceMonitorRetry", "retry ~Value");
                SetRule("ServiceMonitorTimeout", "timeout ~Value");
                SetRule("ServiceMonitorType", "type off|ping|connect");
                SetRule("ServiceMonitorPort", "port ~Value");
                SetRule("ServiceMonitorEmail", "mail (on|off|(to ~Value)|(alertfreq ~Value))");
                SetOrderRule("ServiceMonitorOrder", "ServiceMonitorInterval ServiceMonitorRetry ServiceMonitorTimeout ServiceMonitorType ServiceMonitorPort ServiceMonitorEmail", 1, NULL);
                SetRule("ServiceMonitor", "monitor ~ServiceMonitorOrder");
            }
            SetOrderRule("ServiceVS", "ServiceSchedule ServiceMonitor ServicePersistent", 1, NULL);
        }
        {   //Server
            SetRule("ServerName", "name ~Value");
            SetRule("ServerIP", "ip ~Value");
            SetRule("ServerWeight", "weight ~Value");
            SetRule("ServerAction", "action route|masq|tunnel");
            SetRule("ServerMapPort", "mapport ~Value");
            SetOrderRule("ServerAddOP", "ServerName ServerIP ServerWeight ServerAction ServerMapPort", 0, "ServerIP");
            SetOrderRule("ServerSetOP", "ServerName ServerIP ServerWeight ServerAction ServerMapPort", 1, NULL);
            SetRule("AddServer", "~ServerAddOP ?(disable|enable)");
            SetRule("ServerSet", "enable|disable|~ServerSetOP");
            SetRule("ServiceServer", "server (add ~AddServer)|flush|(del ~Value)|(~Value ~ServerSet)");
        }
        {   //Traffic
            SetRule("ServiceTrafficUp", "up ~Value");
            SetRule("ServiceTrafficDown", "down ~Value");
            SetOrderRule("ServiceTrafficOrder", "ServiceTrafficUp ServiceTrafficDown", 1, NULL);
            SetRule("ServiceTraffic", "traffic ~ServiceTrafficOrder");
        }
        SetRule("ServiceSet", "?list|enable|disable|~Description|(name ~Value)|(ha local|self|other)|(address ~ServiceAddress)|~ServiceVS|~ServiceServer|~ServiceTraffic");
        SetRule("Service", "vs ?list|(add ?~Value)|(del ~Value)|(~Value ~ServiceSet)");
    }
    {   //HA
        SetValueRule("ServiceDev", GetServiceDev);
        SetRule("HAPingGroup", "group ~Value *~Value");
        SetRule("HAPingGroupList", "~HAPingGroup *~HAPingGroup");
        SetRule("HAPing", "ping flush|~HAPingGroupList");
        SetRule("HASendMail", "mail (on|off|(to ~Value))");
        SetRule("HAKeepalive", "keepalive ~Value");
        SetRule("HADeadtime", "deadtime ~Value");
        SetRule("HAWarntime", "warntime ~Value");
        SetRule("HAInitdead", "initdead ~Value");
        SetOrderRule("HATime", "HAKeepalive HAWarntime HADeadtime HAInitdead HASendMail", 1, NULL);
        SetRule("HAPort", "port ~Value");
        SetRule("HAIP", "ip ~Value");
        SetRule("HADev", "dev ~ServiceDev");
        SetRule("HAHostname", "hostname ~Value");
        SetOrderRule("HAOther", "HAPort HAIP HADev HAHostname", 1, NULL);
        SetRule("HASet", "~HATime|~HAOther|(autoback yes|no)|~HAPing|(sync off|~Value)");
        SetRule("HA", "ha ?(show|enable|disable|~HASet)");
    }
    {   //System
        SetRule("Licence", "licence ?(machine|(register ~Value))");
        SetRule("System", "system reboot|shutdown|(configure save|reload)|(status cpu|disk|memory)|(time (?get)|set)|(password ?set)|(log (?status|(host ~Value)|enable|disable))|(mail (?status|enable|disable|(set address ~Value smtp ~Value user ~Value passwd ~Value)))|~Licence");
    }
    {   //Bind
        SetRule("Alias", "alias ~Value *~Value");
        SetRule("ListStart", "~Value *~Value");
        SetRule("ListSecond", "(isp ~Value)|noisp ~ListStart");
        SetRule("IPList", "server ?(isp ~Value) ~ListStart *~ListSecond");
        SetRule("TTL", "ttl ~Value");
        SetRule("Enabled", "enable|disable");
        SetRule("ReturnAll", "all|link");
        SetRule("AName", "domain ~Value");
        SetOrderRule("SetBindOrderService", "TTL ReturnAll Enabled AName", 1, NULL);
        SetRule("SetBindService", "~Alias|~IPList|~SetBindOrderService");
        SetRule("Domain", "domain (add ~Value)|(set ~Value ~SetBindService)|(del ~Value)");
        SetRule("Bind", "dns ?(~Enabled|(port ~Value)|(reverse ~Enabled)|~Domain|show)");
    }
    {   //ISP
        SetRule("Set", "set ~Value (name ~Value)|(net ~Value *~Value)");
        SetRule("Del", "del ~Value");
        SetRule("Add", "add ~Value net ~Value *~Value");
        SetRule("ISP", "isp ?(show ?all)|~Add|~Del|~Set");
    }
    {   //BlackList
        SetOrderRule("BlackListOrderMatch", "RuleProt RuleFrom RuleTo", 1, NULL);
        SetRule("BlackListMatch", "any|~BlackListOrderMatch");
        SetRule("BlackList", "blacklist ?list|(add match ~BlackListMatch)|(del ~Value)|(~Value (match ~BlackListMatch)|~Description)");
    }
    {   //HTTP
        SetValueRule("HttpGroupGet", GetHttpGroup);
        SetRule("Http", "http ?list|~HttpInfo|~HttpService|~HttpGroup");
        {   //Service
            SetRule("HttpService", "service ?list|(del ~Value)|(add ~HttpServiceInfoAdd)|(~Value ?(get|~HttpServiceInfoSet|~HttpLocation))");
            SetRule("HttpServiceInfoAdd", "name ~Value ip ~Value dev ~AllDev port ~Value");
            SetRule("HttpServiceInfoSet", "(name ~Value)|(ip ~Value)|(dev ~AllDev)|(port ~Value)|(ssl on|off)|(timeout ~Value)|(ha local|self|other)|(cookie enabled|disabled|(timeout ~Value)|(name ~Value))");
            SetRule("HttpLocation", "location (del ~Value)|(add match ~Value group ~HttpGroupGet cachevalid ~Value)|(insert ~Value match ~Value group ~HttpGroupGet)");
        }
        {
            SetRule("HttpGroup", "group ?list|(del ~Value)|(add ~HttpGroupInfoAdd)|(~Value ?(get|~HttpGroupInfoSet|~HttpServer))");
            SetRule("HttpGroupInfoAdd", "name ~Value ~HttpMethod");
            SetRule("HttpGroupInfoSet", "(name ~Value)|~HttpMethod");
            SetRule("HttpMethod", "method rr|wrr|iphash|fair|urlhash|cookie|cookie-tomcat|cookie-resin|proxy-iphash");
            SetRule("HttpServer", "server (add ~HttpServerAdd)|(del ~Value)|(~Value ~HttpServerSet)");
            SetRule("HttpServerAdd", "ip ~Value port ~Value");
            SetRule("HttpServerSet", "(ip ~Value)|(port ~Value)|(weight ~Value)|(type backup|down|normal)|(fail ~Value)|(timeout ~Value)|(srunid ~Value)");
        }
        {
            SetRule("HttpInfo", "get|enabled|disabled|(processor ~Value)|(connections ~Value)|(keepalive ~Value)|(cacheinactive ~Value)|(gzip on|off|(length ~Value))|(denynotmatch on|off)");
        }
    }
#ifdef __DEBUG__
    {   //Debug
        {   //Statistic
            SetRule("StatisticFrom", "from ~Value");
            SetRule("StatisticTo", "to ~Value");
            SetRule("StatisticInterval", "interval 60|3600|86400|604800");
            SetOrderRule("StatisticOption", "StatisticFrom StatisticTo StatisticInterval", 3, NULL);
            SetRule("Statistic", "statistic service|server ~Value ~StatisticOption");
        }
        {   //Configure
            SetRule("Configure", "configure import|export");
        }
        {   //HttpCA
            SetRule("HttpTest", "http importCATest ~Value");
        }
        SetRule("Debug", "debug time|~Statistic|~Configure|~HttpTest");
    }
#else
#endif
    SetRule("Root",
#ifdef __DEBUG__
            "~Debug|"
#endif
            "~Interface|~Route|~Nat|~Arp|~NetworkOther|~Adsl|~Service|~HA|~System|~Bind|~ISP|~BlackList|~Http");
#endif
    Init();
    PRINTF("Init Rule done.");
}
