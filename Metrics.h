#ifndef _METRICS_H_
#define _METRICS_H_

// by yuwf qingting.water@gmail.com

/* 使用案例

将数据添加到记录中，使用 g_metricsrecord.Snapshot(Measure::Prometheus) 获取格式化的快照数据
Measure("metricsname").Tag("tag", 12).Add(1);
Measure("metricsname").Tag("tag", "id").Set(10);
Measure("metricsname").Tag("tag", "max").Max(10);

下面这个中方式效率非常高，可以考虑在热点地方使用，提高性能
Metrics* pMetrics = Measure("metricsname").Tag("tag", 12).Reg(); // pMetrics对象保存下来，在其他地方使用
pMetrics->Add(1);


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
	Metrics(const std::string& n, const std::map<std::string, std::string>& t)
		: name(n)
		, tags(t)
	{}
	Metrics(const Metrics& other)
		: value(other.value.load())
		, name(other.name)
		, tags(other.tags)
	{
	}

	void Add(int64_t v) { value += v; }
	void Set(int64_t v) { value = v; }
	void Max(int64_t v) { if (value < v) value = v; }

	std::atomic<int64_t> value = { 0 };

	// 构造时赋值，不可修改
	const std::string name;
	const std::map<std::string, std::string> tags;
};

struct MetricsKey
{
	std::size_t hash = 0;		// key的hash值 key有Measure组建
	std::size_t hash2 = 0;		// 使用两层hash 减少碰撞

	bool operator == (const MetricsKey& other) const
	{
		return hash == other.hash && hash2 == other.hash2;
	}
	bool operator < (const MetricsKey& other) const
	{
		return hash < other.hash;
	}
};
struct MetricsKeyHash
{
	std::size_t operator()(const MetricsKey& obj) const
	{
		return obj.hash;
	}
};

typedef std::unordered_map<MetricsKey, Metrics*, MetricsKeyHash> MetricsMap;


// 测量工具 生成指标
// 为了效率 不检查Tag中特殊格式的存在 如 空格 ' " , { } \0
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

	// 获取指标对象
	Metrics* Reg();

	// 直接操作指标数据
	Metrics* Add(int64_t v);
	Metrics* Set(int64_t v);
	Metrics* Max(int64_t v);

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
	// 注册指标 返回指标指针
	Metrics* Reg(const Measure& measure);

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