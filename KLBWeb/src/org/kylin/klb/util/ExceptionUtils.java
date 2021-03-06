package org.kylin.klb.util;
import java.util.HashMap;
import java.util.Map;

public class ExceptionUtils {

	private Map<Integer,String> _format = new HashMap();
	private Map<Integer,Integer> _count = new HashMap();
	
	private static ExceptionUtils instance = null;

	private ExceptionUtils() {
		_format.put(200, "命令不匹配.");
		_count.put(200, 0);
		_format.put(201, "无法连接KLB管理程序.");
		_count.put(201, 0);
		_format.put(202, "%s 为无效操作符.");
		_count.put(202, 1);
		_format.put(203, "%s 不是有效整数.");
		_count.put(203, 1);
		_format.put(204, "两次输入的密码不一致.");
		_count.put(204, 0);
		_format.put(205, "时间格式不正确.");
		_count.put(205, 0);
		_format.put(206, "时区格式不正确.");
		_count.put(206, 0);
		_format.put(207, "接口 %s 不存在IP %s.");
		_count.put(207, 2);
		_format.put(208, "ISP名称 %s 不存在.");
		_count.put(208, 1);
		_format.put(500, "密码无效.");
		_count.put(500, 0);
		_format.put(501, "%s 参数无效.");
		_count.put(501, 1);
		_format.put(502, "版本不匹配.");
		_count.put(502, 0);
		_format.put(503, "Licence无效，请注册.");
		_count.put(503, 0);
		_format.put(505, "%s 为无效Licence.");
		_count.put(505, 1);
		_format.put(1000, "整数 %s 值无效.");
		_count.put(1000, 1);
		_format.put(1001, "%s 值超出枚举范围.");
		_count.put(1001, 1);
		_format.put(1002, "字符串 %s 格式无效.");
		_count.put(1002, 1);
		_format.put(1003, "索引 %s 超过范围.");
		_count.put(1003, 1);
		_format.put(1004, "获取列表的起始参数 %s 无效.");
		_count.put(1004, 1);
		_format.put(1005, "获取列表的长度参数 %s 无效.");
		_count.put(1005, 1);
		_format.put(1006, "二进制数据过长.");
		_count.put(1006, 0);
		_format.put(2000, "导入配置文件失败.");
		_count.put(2000, 0);
		_format.put(2001, "配置文件保存失败.");
		_count.put(2001, 0);
		_format.put(2002, "设置时间的参数无效.");
		_count.put(2002, 0);
		_format.put(3000, "接口 %s 不存在.");
		_count.put(3000, 1);
		_format.put(3001, "接口 %s 已存在.");
		_count.put(3001, 1);
		_format.put(3002, "接口 %s 设置MTU %s 失败.");
		_count.put(3002, 2);
		_format.put(3003, "开启接口 %s 失败.");
		_count.put(3003, 1);
		_format.put(3004, "关闭接口 %s 失败.");
		_count.put(3004, 1);
		_format.put(3005, "接口 %s 设置物理地址 %s 失败.");
		_count.put(3005, 2);
		_format.put(3100, "清除接口 %s IP地址失败.");
		_count.put(3100, 1);
		_format.put(3101, "接口 %s 添加IP %s 失败.");
		_count.put(3101, 2);
		_format.put(3102, "接口 %s 已开启DHCP，无法设置IP.");
		_count.put(3102, 1);
		_format.put(3103, "接口 %s 已存在IP %s ，设置失败.");
		_count.put(3103, 2);
		_format.put(3201, "接口 %s 开启协商失败.");
		_count.put(3201, 1);
		_format.put(3202, "接口 %s 关闭协商失败.");
		_count.put(3202, 1);
		_format.put(3203, "接口 %s 指定工作模式失败.");
		_count.put(3203, 1);
		_format.put(3204, "接口 %s 为接口 %s 的从接口，设置失败.");
		_count.put(3204, 2);
		_format.put(3205, "接口 %s 已属于PPPoE接口 %s ，设置失败.");
		_count.put(3205, 2);
		_format.put(3301, "添加聚合接口 %s 失败.");
		_count.put(3301, 1);
		_format.put(3302, "删除聚合接口 %s 失败.");
		_count.put(3302, 1);
		_format.put(3303, "在接口 %s 添加从接口 %s 失败.");
		_count.put(3303, 2);
		_format.put(3304, "在接口 %s 删除从接口 %s 失败.");
		_count.put(3304, 2);
		_format.put(3305, "接口 %s 无从接口.");
		_count.put(3305, 1);
		_format.put(3306, "在接口 %s 添加检测IP %s 失败.");
		_count.put(3306, 2);
		_format.put(3307, "在接口 %s 删除检测IP %s 失败.");
		_count.put(3307, 2);
		_format.put(3308, "接口 %s 设置聚合模式失败.");
		_count.put(3308, 1);
		_format.put(3309, "从接口 %s 不支持802.3ad模式.");
		_count.put(3309, 1);
		_format.put(3310, "从接口 %s 不支持alb平衡模式.");
		_count.put(3310, 1);
		_format.put(3311, "从接口 %s 不支持tlb平衡模式.");
		_count.put(3311, 1);
		_format.put(3312, "接口 %s 设置链路检查模式失败.");
		_count.put(3312, 1);
		_format.put(3401, "物理接口 %s 已被使用.");
		_count.put(3401, 1);
		_format.put(3402, "物理接口 %s 无法启动.");
		_count.put(3402, 1);
		_format.put(3403, "PPPoE接口 %s 正忙.");
		_count.put(3403, 1);
		_format.put(3404, "PPPoE接口 %s 的用户名为空.");
		_count.put(3404, 1);
		_format.put(3405, "PPPoE接口 %s 的密码为空.");
		_count.put(3405, 1);
		_format.put(3406, "PPPoE接口 %s 正在拨号.");
		_count.put(3406, 1);
		_format.put(3407, "PPPoE接口 %s 已连接.");
		_count.put(3407, 1);
		_format.put(3408, "PPPoE接口 %s 已停止.");
		_count.put(3408, 1);
		_format.put(4000, "数据上传失败.");
		_count.put(4000, 0);
		_format.put(4001, "初始化升级失败.");
		_count.put(4001, 0);
		_format.put(4002, "系统升级失败.");
		_count.put(4002, 0);
		_format.put(5000, "检测网关 %s 所属接口失败.");
		_count.put(5000, 1);
		_format.put(5001, "网关接口不能为空.");
		_count.put(5001, 0);
		_format.put(5100, "路由目标不能为空.");
		_count.put(5100, 0);
		_format.put(5101, "路由 %s 度量值 %s 已存在");
		_count.put(5101, 2);
		_format.put(5102, "路由 %s 度量值 %s 不存在");
		_count.put(5102, 2);
		_format.put(5200, "策略路由条目过多.");
		_count.put(5200, 0);
		_format.put(6000, "映射IP不能为空.");
		_count.put(6000, 0);
		_format.put(6001, "映射的端口范围 %s-%s 无效.");
		_count.put(6001, 2);
		_format.put(7000, "主机名不能为空.");
		_count.put(7000, 0);
		_format.put(7001, "DNS服务器配置失败.");
		_count.put(7001, 0);
		_format.put(7002, "%s 不是有效的协议名称或编号.");
		_count.put(7002, 1);
		_format.put(7003, "%s 协议不支持使用端口.");
		_count.put(7003, 1);
		_format.put(7004, "%s 不是有效的端口名称或端口号.");
		_count.put(7004, 1);
		_format.put(8000, "虚拟服务地址重复.");
		_count.put(8000, 0);
		_format.put(8001, "虚拟服务的IP不能为空.");
		_count.put(8001, 0);
		_format.put(8002, "虚拟服务的IP %s 已存在.");
		_count.put(8002, 1);
		_format.put(8003, "虚拟服务名称 %s 已存在.");
		_count.put(8003, 1);
		_format.put(8004, "虚拟服务名称 %s 无效.");
		_count.put(8004, 1);
		_format.put(8005, "虚拟服务名称 %s 未找到.");
		_count.put(8005, 1);
		_format.put(8006, "虚拟服务名称为空.");
		_count.put(8006, 0);
		_format.put(8007, "虚拟服务的标记值 %s 已存在.");
		_count.put(8007, 1);
		_format.put(8008, "虚拟服务数量过多.");
		_count.put(8008, 0);
		_format.put(8009, "虚拟服务端口范围 %s-%s 无效.");
		_count.put(8009, 2);
		_format.put(8100, "服务器名称 %s 已存在.");
		_count.put(8100, 1);
		_format.put(8101, "服务器名称 %s 无效.");
		_count.put(8101, 1);
		_format.put(8102, "服务器名称 %s 未找到.");
		_count.put(8102, 1);
		_format.put(8103, "服务器名称为空.");
		_count.put(8103, 0);
		_format.put(8104, "服务器IP %s 已存在.");
		_count.put(8104, 1);
		_format.put(8105, "服务器IP不能为空.");
		_count.put(8105, 0);
		_format.put(8106, "服务器ID %s 已存在.");
		_count.put(8106, 1);
		_format.put(8200, "高可用模块正在运行.");
		_count.put(8200, 0);
		_format.put(8201, "心跳检查时间参数配置错误.");
		_count.put(8201, 0);
		_format.put(8202, "心跳接口不能为空.");
		_count.put(8202, 0);
		_format.put(8203, "心跳接口 %s 无效.");
		_count.put(8203, 1);
		_format.put(8204, "心跳对方的IP不能为空.");
		_count.put(8204, 0);
		_format.put(8205, "心跳对方的主机名不能为空.");
		_count.put(8205, 0);
		_format.put(8300, "数据统计的时间范围 %s-%s 无效.");
		_count.put(8300, 2);
		_format.put(8301, "数据统计间隔 %s 无效.");
		_count.put(8301, 1);
		_format.put(8302, "虚拟服务ID %s 无效.");
		_count.put(8302, 1);
		_format.put(8303, "服务器IP %s 无效.");
		_count.put(8303, 1);
		_format.put(9000, "删除ARP项失败.");
		_count.put(9000, 0);
		_format.put(9001, "设置静态ARP项失败.");
		_count.put(9001, 0);
		_format.put(10000, "域名不能为空.");
		_count.put(10000, 0);
		_format.put(10001, "域名 %s 不存在.");
		_count.put(10001, 1);
		_format.put(10002, "域名 %s 已存在.");
		_count.put(10002, 1);
		_format.put(10100, "别名不能为空.");
		_count.put(10100, 0);
		_format.put(10101, "别名或域名 %s 已存在.");
		_count.put(10101, 1);
		_format.put(10200, "服务器IP不能为空.");
		_count.put(10200, 0);
		_format.put(10201, "服务器 %s 的ISP %s 不存在.");
		_count.put(10201, 2);
		_format.put(11000, "无法找到ISP，ID %s .");
		_count.put(11000, 1);
		_format.put(11001, "ISP数量过多.");
		_count.put(11001, 0);
		_format.put(11002, "ISP名称不能为空.");
		_count.put(11002, 0);
		_format.put(11003, "ISP名称 %s 已使用.");
		_count.put(11003, 1);
		_format.put(11004, "ISP目标网段不能为空.");
		_count.put(11004, 0);
	}
	
	public static synchronized ExceptionUtils getInstance() {
		if (instance == null)
			instance = new ExceptionUtils();
		return instance;
	}
	
	static public String GetFormat(int code) {
		return getInstance()._format.get(code);
	}
	
	static public int GetCount(int code) {		
		return getInstance()._count.get(code);
	}
	
	static public boolean IsExist(int code) {
		return getInstance()._format.containsKey(code);
	}
	
}
