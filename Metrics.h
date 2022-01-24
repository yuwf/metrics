#ifndef _METRICS_H_
#define _METRICS_H_

// by yuwf qingting.water@gmail.com

/* 使用案例

将数据添加到记录中，使用 g_metricsrecord.Snapshot(Measure::Prometheus) 获取格式化的快照数据
Measure("metricsname").Tag("tag", 12).Add(1);
Measure("metricsname").Tag("tag", "id").Set(10);
Measure("metricsname").Tag("tag", "max").Max(10);

直接获取格式化的快照数据
std::string str = Measure("metricsname").Tag("tag", 12).Snapshot(1, Measure::Prometheus);
std::string str = Measure("metricsname").Tag("tag", "id").Snapshot(10, Measure::Prometheus);
std::string str = Measure("metricsname").Tag("tag", "max").Snapshot(10, Measure::Prometheus);

*/

#include <string>
#include <map>
#include <unordered_map>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <atomic>

struct Metrics
{
	Metrics(){}
	Metrics(const Metrics& other)
		: value(other.value.load())
		, name(other.name)
		, tags(other.tags)
	{
	}

	std::atomic<int64_t> value = { 0 };

	std::string name;
	std::map<std::string, std::string> tags;
};

struct MetricsKey
{
	std::size_t hash = 0;		// key的hash值 key有Measure组建
	std::size_t hash2 = 0;		// 使用两层hash 减少碰撞

	bool operator == (const MetricsKey& other) const
	{
		return hash == other.hash && hash2 == other.hash2;
	}
};
struct MetricsKeyHash
{
	std::size_t operator()(const MetricsKey& obj) const
	{
		return obj.hash;
	}
};

typedef std::unordered_map<MetricsKey, Metrics, MetricsKeyHash> MetricsMap;


// 测量工具 生成指标
// 为了效率 转化Tag中的特殊字符 如 空格 ' " , { } \0
// 需要外部使用者自己防范，否则会出现格式错误，尤其是空格
class Measure
{
public:
	Measure(const std::string& metricsname);
	Measure(const char* metricsname);
	~Measure();

	enum SnapshotType { Json, Influx, Prometheus };
	enum OpType { OpAdd, OpSet, OpMax, };

	Measure& Tag(const char* name, const char* value);
	Measure& Tag(const char* name, int value);
	Measure& Tag(const char* name, int64_t value);
	Measure& Tag(const std::string& name, const std::string& value);
	Measure& Tag(const std::string& name, int value);
	Measure& Tag(const std::string& name, int64_t value);

	// 下面的函数都是结束操作=========================================

	// 数据操作 保存到MeasureRecord中
	void Add(int64_t v);
	void Set(int64_t v);
	void Max(int64_t v);

	// 数据直接转化成快照数据
	std::string Snapshot(int64_t v, SnapshotType type);
protected:
	friend class MetricsRecord;
	char* buff = NULL;
	int size = 0;
	int curpos = 0;

	void Write(void const* data, int len);

private:
	// 禁止拷贝
	Measure(const Measure&) = delete;
	Measure& operator=(const Measure&) = delete;
};

class MetricsRecord
{
public:
	// 指标数据添加到记录中 measure的数据将会情况
	
	void Add(const Measure& measure, Measure::OpType op, int64_t v);

	// 获取指标数据
	std::string Snapshot(Measure::SnapshotType type);

	void SetRecord(bool b) { brecord = b; }

protected:
	boost::shared_mutex mutex;
	MetricsMap records;

	// 是否记录测试数据
	bool brecord = true;
};

extern MetricsRecord g_metricsrecord;

#endif