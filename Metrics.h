#ifndef _METRICS_H_
#define _METRICS_H_

#include <string>
#include <map>
#include <iosfwd>
#include <unordered_map>
#include <boost/thread/mutex.hpp>
#include <atomic>

//是否Influxdb格式 否则就是Prometheus格式
#define INFLUXDB 0

struct MetricsValue
{
	MetricsValue(){}
	MetricsValue(const MetricsValue& other)
		: value(other.value.load())
	{
	}

	std::atomic<int64_t> value = { 0 };

	MetricsValue& operator = (const MetricsValue& other)
	{
		value = other.value.load();
		return *this;
	}
};

struct MetricsKey
{
	std::size_t hash = 0;		// key的hash值
	std::string key;

	bool operator == (const MetricsKey& other) const
	{
		return hash == other.hash && key == other.key;
	}
};

struct MetricsKeyHash
{
	std::size_t operator()(const MetricsKey& obj) const
	{
		return obj.hash;
	}
};

typedef std::unordered_map<MetricsKey, MetricsValue, MetricsKeyHash> MetricsMap;


// 测量工具 生成指标
// 为了效率 不检查TagKey和TagValue中有 ",空格,{,} 等特殊格式的存在
class Measure
{
public:
	template<class MetricsName>
	Measure(const MetricsName& metricsname)
	{
		ss << metricsname;
	}

	template<class TagKey, class TagValue>
	Measure& Tag(const TagKey& name, const TagValue& value)
	{
#if INFLUXDB
		ss << "," << name << "=" << value;
#else
		ss << (!bwritetag ? "{" : ",") << name << "=\"" << value << "\"";
#endif
		bwritetag = true;
		return *this;
	}

	// 下面的函数都是结束操作=========================================

	// 数据操作 保存到MeasureRecord中
	void Add(int64_t v);
	void Set(int64_t v);
	void Max(int64_t v);

	// 数据直接转化成快照数据
	std::string Snapshot(int64_t v);

	std::ostringstream ss;

protected:
	bool bwritetag = false;	// 是否写入了tag
	void onTagEnd();
};

enum class MeasureOp
{
	Add,
	Set,
	Max,
};

class MetricsRecord
{
public:
	void Add(const Measure& measure, MeasureOp op, int64_t v);

	// 获取指标数据 并清除保留的数据
	std::string Snapshot();

	void SetRecord(bool b) { brecord = b; }

protected:
	boost::shared_mutex mutex;
	MetricsMap records;

	// 是否记录测试数据
	bool brecord = true;
};

extern MetricsRecord g_metricsrecord;

#endif