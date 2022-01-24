#include "Metrics.h"
#include <vector>
#include <regex>
#include <iosfwd>

MetricsRecord g_metricsrecord;

Measure::Measure(const std::string& metricsname)
{
	Write(metricsname.data(), metricsname.length());
}

Measure::Measure(const char* metricsname)
{
	Write(metricsname, strlen(metricsname));
}

Measure::~Measure()
{
	if (buff != NULL)
	{
		delete[] buff;
	}
}

Measure& Measure::Tag(const char* name, const char* value)
{
	Write(" ", 1);
	Write(name, strlen(name));
	Write(" ", 1);
	Write(value, strlen(value));
	return *this;
}

Measure& Measure::Tag(const char* name, int value)
{
	return Tag(name, std::to_string(value).c_str());
}

Measure& Measure::Tag(const char* name, int64_t value)
{
	return Tag(name, std::to_string(value).c_str());
}

Measure& Measure::Tag(const std::string& name, const std::string& value)
{
	Write(" ", 1);
	Write(name.data(), name.length());
	Write(" ", 1);
	Write(value.data(), value.length());
	return *this;
}

Measure& Measure::Tag(const std::string& name, int value)
{
	return Tag(name, std::to_string(value));
}

Measure& Measure::Tag(const std::string& name, int64_t value)
{
	return Tag(name, std::to_string(value));
}

void Measure::Add(int64_t v)
{
	g_metricsrecord.Add(*this, OpAdd, v);
}

void Measure::Set(int64_t v)
{
	g_metricsrecord.Add(*this, OpSet, v);
}

void Measure::Max(int64_t v)
{
	g_metricsrecord.Add(*this, OpMax, v);
}

std::string Measure::Snapshot(int64_t v, SnapshotType type)
{
	std::regex re{ " " };
	std::vector<std::string> output = std::vector<std::string>{ std::cregex_token_iterator(buff, &buff[curpos], re, -1), std::cregex_token_iterator() };
	if (output.empty())
	{
		return "";
	}
	std::ostringstream ss;
	if (type == Json)
	{
		ss << "{";
		ss << "\"name\":\"" << output.front() << "\",";
		for (std::size_t i = 1; (i + 1) < output.size(); i += 2)
		{
			ss << "\"" << output[i] << "\":\"" << output[i+1] << "\",";
		}
		ss << "\"value\":" << v << "";
		ss << "}";
	}
	else if (type == Influx)
	{
		ss << output.front();
		for (std::size_t i = 1; (i + 1) < output.size(); i += 2)
		{
			ss << "," << output[i] << "=" << output[i + 1];
		}
		ss << " value=" << v << "i\n";
	}
	else if (type == Prometheus)
	{
		ss << output.front();
		if (output.size() >= 3)
		{
			for (std::size_t i = 1; (i + 1) < output.size(); i += 2)
			{
				ss << (i == 1 ? "{" : ",");
				ss << output[i] << "=\"" << output[i + 1] << "\"";
			}
			ss << "}";
		}
		ss << " " << v << "\n";
	}
	return ss.str();
}

void Measure::Write(void const* data, int len)
{
	if (data == NULL || len <= 0)
	{
		return;
	}
	const static int buff_size = 128;

	if ((curpos + len) > size - 1) // 这里-1用来控制最后一个字符为\0 直接可以当做字符串来使用
	{
		size = buff_size*((curpos + len) / buff_size + 1);
		char* p = new char[size];
		memset(p, 0, size);

		if (buff)
		{
			memcpy(p, buff, curpos);
			delete[] buff;
		}
		buff = p;
	}
	memcpy(buff + curpos, data, len);
	curpos += len;
}

// BKDR Hash Function
static unsigned int BKDRHash(char *str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}
	return (hash & 0x7FFFFFFF);
}

// AP Hash Function
static unsigned int APHash(char *str)
{
	unsigned int hash = 0;
	int i;

	for (i = 0; *str; i++)
	{
		if ((i & 1) == 0)
		{
			hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
		}
		else
		{
			hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
		}
	}
	return (hash & 0x7FFFFFFF);
}

void MetricsRecord::Add(const Measure& measure, Measure::OpType op, int64_t v)
{
	if (!brecord )
	{
		return;
	}

	MetricsKey key;
	key.hash = BKDRHash(measure.buff);
	key.hash2 = APHash(measure.buff);

	// 先用共享锁 如果存在直接修改
	{
		boost::shared_lock<boost::shared_mutex> lock(mutex);
		auto it = records.find(key);
		if (it != records.end())
		{
			if (op == Measure::OpAdd)
			{
				it->second.value += v;
			}
			else if (op == Measure::OpSet)
			{
				it->second.value = v;
			}
			else if (op == Measure::OpMax)
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
		std::regex re{ " " };
		std::vector<std::string> output = std::vector<std::string>{ std::cregex_token_iterator(measure.buff, &measure.buff[measure.curpos], re, -1), std::cregex_token_iterator() };
		if (output.empty())
		{
			return;
		}

		boost::unique_lock<boost::shared_mutex> lock(mutex);
		Metrics& value = records[key];
		value.value = v;
		value.name = output.front();
		for (std::size_t i = 1; (i+1) < output.size(); i += 2)
		{
			value.tags[output[i]] = output[i + 1];
		}
	}
}

std::string MetricsRecord::Snapshot(Measure::SnapshotType type)
{
	MetricsMap lastdata;
	{
		boost::unique_lock<boost::shared_mutex> lock(mutex);
		lastdata = records;
	}

 	std::ostringstream ss;
	if (type == Measure::Json)
	{
		ss << "[";
		int index = 0;
		for (const auto& it : lastdata)
		{
			ss << ((++index) == 1 ? "{" : ",{");
			ss << "\"name\":\"" << it.second.name  << "\",";
			for (const auto& t : it.second.tags)
			{
				ss << "\"" << t.first << "\":\"" << t.second << "\",";
			}
			ss << "\"value\":" << it.second.value << "";
			ss << "}";
		}
		ss << "]";
	}
	else if (type == Measure::Influx)
	{
		for (const auto& it : lastdata)
		{
			ss << it.second.name;
			for (const auto& t : it.second.tags)
			{
				ss << "," << t.first << "=" << t.second;
			}
			ss << " value=" << it.second.value << "i\n";
		}
	}
	else if (type == Measure::Prometheus)
	{
		for (const auto& it : lastdata)
		{
			ss << it.second.name;
			if (!it.second.tags.empty())
			{
				ss << "{";
				int index = 0;
				for (const auto& t : it.second.tags)
				{
					ss << ((++index) == 1 ? "" : ",");
					ss << t.first << "=\"" << t.second << "\"";
				}
				ss << "}";
			}
			ss << " " << it.second.value << "\n";
		}
	}
	return ss.str();
}
