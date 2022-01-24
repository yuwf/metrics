#include "Metrics.h"

MetricsRecord g_metricsrecord;

void Measure::Add(int64_t v)
{
	onTagEnd();
	g_metricsrecord.Add(*this, MeasureOp::Add, v);
}

void Measure::Set(int64_t v)
{
	onTagEnd();
	g_metricsrecord.Add(*this, MeasureOp::Set, v);
}

void Measure::Max(int64_t v)
{
	onTagEnd();
	g_metricsrecord.Add(*this, MeasureOp::Max, v);
}

std::string Measure::Snapshot(int64_t v)
{
	onTagEnd();
#if INFLUXDB
	ss << " value=" << v << "i\n";
#else
	ss << " " << v << "\n";
#endif
	return ss.str();
}

void Measure::onTagEnd()
{
#if INFLUXDB
#else
	if (bwritetag)
		ss << "}";
#endif
}

void MetricsRecord::Add(const Measure& measure, MeasureOp op, int64_t v)
{
	if (!brecord )
	{
		return;
	}
	MetricsKey key;
	key.key = measure.ss.str();
	key.hash = std::hash<std::string>()(key.key);
	// 先用共享锁 如果存在直接修改
	{
		boost::shared_lock<boost::shared_mutex> lock(mutex);
		auto it = records.find(key);
		if (it != records.end())
		{
			if (op == MeasureOp::Add)
			{
				it->second.value += v;
			}
			else if (op == MeasureOp::Set)
			{
				it->second.value = v;
			}
			else if (op == MeasureOp::Max)
			{
				if (it->second.value < v)
				{
					it->second.value = v;
				}
			}
			return; // 直接返回
		}
	}
	// 不存在直接用写锁
	{
		boost::unique_lock<boost::shared_mutex> lock(mutex);
		MetricsValue& value = records[key];
		value.value = v;
	}
}

std::string MetricsRecord::Snapshot()
{
	MetricsMap lastdata;
	{
		boost::unique_lock<boost::shared_mutex> lock(mutex);
		lastdata = records;
	}

	std::ostringstream ss;
	for (const auto& m : lastdata)
	{
#if INFLUXDB
		ss << m.first.key << " value=" << m.second.value << "i\n";
#else
		ss << m.first.key << " " << m.second.value << "\n";
#endif
	}

	return ss.str();
}
