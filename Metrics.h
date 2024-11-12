#ifndef _METRICS_H_
#define _METRICS_H_

// by git@github.com:yuwf/metrics.git

/* 使用案例

将数据添加到记录中，使用 g_metricsrecord.Snapshot(Measure::Prometheus) 获取格式化的快照数据
Measure("metricsname").Tag("tag", 12).Add(1);
Measure("metricsname").Tag("tag", "id").Set(10);
Measure("metricsname").Tag("tag", "max").Max(10);

下面这个中方式效率非常高，可以考虑在热点地方使用，提高性能，要求metricsname和tags都是固定的
Metrics* pMetrics = Measure("metricsname").Tag("tag", "name").Reg();		// pMetrics对象保存下来，在其他地方使用
static Metrics* pMetrics = Measure("metricsname").Tag("tag", "name").Reg();	// 用静态的方式保存pMetrics
pMetrics->Add(1);


直接获取格式化的快照数据
std::string str = Measure("metricsname").Tag("tag", 12).Snapshot(1, Measure::Prometheus);
std::string str = Measure("metricsname").Tag("tag", "id").Snapshot(10, Measure::Prometheus);
std::string str = Measure("metricsname").Tag("tag", "max").Snapshot(10, Measure::Prometheus);

*/

#include <string>
#include <map>
#include <unordered_map>
#include <atomic>
#include <sstream>

// 需要依赖Locker git@github.com:yuwf/locker.git
#include "Locker.h"

#define MetricsTagMaxCount 10

struct MetricsData
{
	MetricsData(const std::string& n, const std::map<std::string, std::string>& t)
		: name(n)
		, tags(t)
	{}

	void Inc() { value += 1; }
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
	struct Hash
	{
		std::size_t operator()(const MetricsKey& obj) const
		{
			return obj.hash;
		}
	};
};

// 指标定位工具，获取指标数据指针或者生成指标快照
class Measure
{
public:
	Measure(const std::string& metricsname);
	Measure(const char* metricsname);
	~Measure();

	enum SnapshotType { Json, Influx, Prometheus };
	enum OpType { OpAdd, OpSet, OpMax, };

	Measure& Tag(const char* name, const char* value);
	Measure& Tag(const char* name, const std::string& value);
	Measure& Tag(const char* name, int value);
	Measure& Tag(const char* name, int64_t value);
	Measure& Tag(const std::string& name, const char* value);
	Measure& Tag(const std::string& name, const std::string& value);
	Measure& Tag(const std::string& name, int value);
	Measure& Tag(const std::string& name, int64_t value);

	// 获取指标对象
	MetricsData* Reg();

	// 直接操作指标数据
	void Add(int64_t v);
	void Set(int64_t v);
	void Max(int64_t v);

	// 数据直接转化成快照数据
	// metricsprefix指标名前缀
	// tags额外添加的标签
	std::string Snapshot(int64_t v, SnapshotType type, const std::string& metricsprefix = "", const std::map<std::string, std::string>& tags = std::map<std::string, std::string>());
protected:
	friend class MetricsRecord;

	// buff
	char fix_buff[128] = { 0 };
	char* buff = fix_buff;
	int size = 128;
	int curpos = 0;

	// tag and value 在buff中的开始位置，结束位置根据下个开始的位置来算
	int tag_value[MetricsTagMaxCount][2];
	int tagcount = 0;

	void Write(void const* data, int len);
	void Write(int data);
	void Write(int64_t data);
	void ExtraReserve(int len);

	// 获取指标的名称 tag
	void NameTagValue(std::string& name, std::map<std::string, std::string>& tags) const;

private:
	// 禁止拷贝
	Measure(const Measure&) = delete;
	Measure& operator=(const Measure&) = delete;
};

class MetricsRecord
{
public:
	// 注册指标 返回指标指针
	MetricsData* Reg(const Measure& measure);

	// 获取指标数据
	// metricsprefix指标名前缀
	// tags额外添加的标签
	std::string Snapshot(Measure::SnapshotType type, const std::string& metricsprefix = "", const std::map<std::string, std::string>& tags = std::map<std::string, std::string>());

	void SetRecord(bool b) { brecord = b; }

protected:
	typedef std::unordered_map<MetricsKey, MetricsData*, MetricsKey::Hash> MetricsMap;
	shared_mutex mutex;
	MetricsMap records;

	// 是否记录测试数据
	bool brecord = true;
};

extern MetricsRecord g_metricsrecord;

#endif