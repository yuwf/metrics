#include "Metrics.h"

// 数字格式化成字符串，参考rapidjson的实现，效率较高
static const char cDigitsLut[200] = {
	'0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
	'1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
	'2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
	'3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
	'4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
	'5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
	'6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
	'7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
	'8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
	'9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
};

static char* u32toa(uint32_t value, char* buffer) {
	if (value < 10000) {
		const uint32_t d1 = (value / 100) << 1;
		const uint32_t d2 = (value % 100) << 1;

		if (value >= 1000)
			*buffer++ = cDigitsLut[d1];
		if (value >= 100)
			*buffer++ = cDigitsLut[d1 + 1];
		if (value >= 10)
			*buffer++ = cDigitsLut[d2];
		*buffer++ = cDigitsLut[d2 + 1];
	}
	else if (value < 100000000) {
		// value = bbbbcccc
		const uint32_t b = value / 10000;
		const uint32_t c = value % 10000;

		const uint32_t d1 = (b / 100) << 1;
		const uint32_t d2 = (b % 100) << 1;

		const uint32_t d3 = (c / 100) << 1;
		const uint32_t d4 = (c % 100) << 1;

		if (value >= 10000000)
			*buffer++ = cDigitsLut[d1];
		if (value >= 1000000)
			*buffer++ = cDigitsLut[d1 + 1];
		if (value >= 100000)
			*buffer++ = cDigitsLut[d2];
		*buffer++ = cDigitsLut[d2 + 1];

		*buffer++ = cDigitsLut[d3];
		*buffer++ = cDigitsLut[d3 + 1];
		*buffer++ = cDigitsLut[d4];
		*buffer++ = cDigitsLut[d4 + 1];
	}
	else {
		// value = aabbbbcccc in decimal

		const uint32_t a = value / 100000000; // 1 to 42
		value %= 100000000;

		if (a >= 10) {
			const unsigned i = a << 1;
			*buffer++ = cDigitsLut[i];
			*buffer++ = cDigitsLut[i + 1];
		}
		else
			*buffer++ = static_cast<char>('0' + static_cast<char>(a));

		const uint32_t b = value / 10000; // 0 to 9999
		const uint32_t c = value % 10000; // 0 to 9999

		const uint32_t d1 = (b / 100) << 1;
		const uint32_t d2 = (b % 100) << 1;

		const uint32_t d3 = (c / 100) << 1;
		const uint32_t d4 = (c % 100) << 1;

		*buffer++ = cDigitsLut[d1];
		*buffer++ = cDigitsLut[d1 + 1];
		*buffer++ = cDigitsLut[d2];
		*buffer++ = cDigitsLut[d2 + 1];
		*buffer++ = cDigitsLut[d3];
		*buffer++ = cDigitsLut[d3 + 1];
		*buffer++ = cDigitsLut[d4];
		*buffer++ = cDigitsLut[d4 + 1];
	}
	return buffer;
}

static char* i32toa(int32_t value, char* buffer) {
	uint32_t u = static_cast<uint32_t>(value);
	if (value < 0) {
		*buffer++ = '-';
		u = ~u + 1;
	}

	return u32toa(u, buffer);
}

static char* u64toa(uint64_t value, char* buffer) {
	const uint64_t  kTen8 = 100000000;
	const uint64_t  kTen9 = kTen8 * 10;
	const uint64_t kTen10 = kTen8 * 100;
	const uint64_t kTen11 = kTen8 * 1000;
	const uint64_t kTen12 = kTen8 * 10000;
	const uint64_t kTen13 = kTen8 * 100000;
	const uint64_t kTen14 = kTen8 * 1000000;
	const uint64_t kTen15 = kTen8 * 10000000;
	const uint64_t kTen16 = kTen8 * kTen8;

	if (value < kTen8) {
		uint32_t v = static_cast<uint32_t>(value);
		if (v < 10000) {
			const uint32_t d1 = (v / 100) << 1;
			const uint32_t d2 = (v % 100) << 1;

			if (v >= 1000)
				*buffer++ = cDigitsLut[d1];
			if (v >= 100)
				*buffer++ = cDigitsLut[d1 + 1];
			if (v >= 10)
				*buffer++ = cDigitsLut[d2];
			*buffer++ = cDigitsLut[d2 + 1];
		}
		else {
			// value = bbbbcccc
			const uint32_t b = v / 10000;
			const uint32_t c = v % 10000;

			const uint32_t d1 = (b / 100) << 1;
			const uint32_t d2 = (b % 100) << 1;

			const uint32_t d3 = (c / 100) << 1;
			const uint32_t d4 = (c % 100) << 1;

			if (value >= 10000000)
				*buffer++ = cDigitsLut[d1];
			if (value >= 1000000)
				*buffer++ = cDigitsLut[d1 + 1];
			if (value >= 100000)
				*buffer++ = cDigitsLut[d2];
			*buffer++ = cDigitsLut[d2 + 1];

			*buffer++ = cDigitsLut[d3];
			*buffer++ = cDigitsLut[d3 + 1];
			*buffer++ = cDigitsLut[d4];
			*buffer++ = cDigitsLut[d4 + 1];
		}
	}
	else if (value < kTen16) {
		const uint32_t v0 = static_cast<uint32_t>(value / kTen8);
		const uint32_t v1 = static_cast<uint32_t>(value % kTen8);

		const uint32_t b0 = v0 / 10000;
		const uint32_t c0 = v0 % 10000;

		const uint32_t d1 = (b0 / 100) << 1;
		const uint32_t d2 = (b0 % 100) << 1;

		const uint32_t d3 = (c0 / 100) << 1;
		const uint32_t d4 = (c0 % 100) << 1;

		const uint32_t b1 = v1 / 10000;
		const uint32_t c1 = v1 % 10000;

		const uint32_t d5 = (b1 / 100) << 1;
		const uint32_t d6 = (b1 % 100) << 1;

		const uint32_t d7 = (c1 / 100) << 1;
		const uint32_t d8 = (c1 % 100) << 1;

		if (value >= kTen15)
			*buffer++ = cDigitsLut[d1];
		if (value >= kTen14)
			*buffer++ = cDigitsLut[d1 + 1];
		if (value >= kTen13)
			*buffer++ = cDigitsLut[d2];
		if (value >= kTen12)
			*buffer++ = cDigitsLut[d2 + 1];
		if (value >= kTen11)
			*buffer++ = cDigitsLut[d3];
		if (value >= kTen10)
			*buffer++ = cDigitsLut[d3 + 1];
		if (value >= kTen9)
			*buffer++ = cDigitsLut[d4];

		*buffer++ = cDigitsLut[d4 + 1];
		*buffer++ = cDigitsLut[d5];
		*buffer++ = cDigitsLut[d5 + 1];
		*buffer++ = cDigitsLut[d6];
		*buffer++ = cDigitsLut[d6 + 1];
		*buffer++ = cDigitsLut[d7];
		*buffer++ = cDigitsLut[d7 + 1];
		*buffer++ = cDigitsLut[d8];
		*buffer++ = cDigitsLut[d8 + 1];
	}
	else {
		const uint32_t a = static_cast<uint32_t>(value / kTen16); // 1 to 1844
		value %= kTen16;

		if (a < 10)
			*buffer++ = static_cast<char>('0' + static_cast<char>(a));
		else if (a < 100) {
			const uint32_t i = a << 1;
			*buffer++ = cDigitsLut[i];
			*buffer++ = cDigitsLut[i + 1];
		}
		else if (a < 1000) {
			*buffer++ = static_cast<char>('0' + static_cast<char>(a / 100));

			const uint32_t i = (a % 100) << 1;
			*buffer++ = cDigitsLut[i];
			*buffer++ = cDigitsLut[i + 1];
		}
		else {
			const uint32_t i = (a / 100) << 1;
			const uint32_t j = (a % 100) << 1;
			*buffer++ = cDigitsLut[i];
			*buffer++ = cDigitsLut[i + 1];
			*buffer++ = cDigitsLut[j];
			*buffer++ = cDigitsLut[j + 1];
		}

		const uint32_t v0 = static_cast<uint32_t>(value / kTen8);
		const uint32_t v1 = static_cast<uint32_t>(value % kTen8);

		const uint32_t b0 = v0 / 10000;
		const uint32_t c0 = v0 % 10000;

		const uint32_t d1 = (b0 / 100) << 1;
		const uint32_t d2 = (b0 % 100) << 1;

		const uint32_t d3 = (c0 / 100) << 1;
		const uint32_t d4 = (c0 % 100) << 1;

		const uint32_t b1 = v1 / 10000;
		const uint32_t c1 = v1 % 10000;

		const uint32_t d5 = (b1 / 100) << 1;
		const uint32_t d6 = (b1 % 100) << 1;

		const uint32_t d7 = (c1 / 100) << 1;
		const uint32_t d8 = (c1 % 100) << 1;

		*buffer++ = cDigitsLut[d1];
		*buffer++ = cDigitsLut[d1 + 1];
		*buffer++ = cDigitsLut[d2];
		*buffer++ = cDigitsLut[d2 + 1];
		*buffer++ = cDigitsLut[d3];
		*buffer++ = cDigitsLut[d3 + 1];
		*buffer++ = cDigitsLut[d4];
		*buffer++ = cDigitsLut[d4 + 1];
		*buffer++ = cDigitsLut[d5];
		*buffer++ = cDigitsLut[d5 + 1];
		*buffer++ = cDigitsLut[d6];
		*buffer++ = cDigitsLut[d6 + 1];
		*buffer++ = cDigitsLut[d7];
		*buffer++ = cDigitsLut[d7 + 1];
		*buffer++ = cDigitsLut[d8];
		*buffer++ = cDigitsLut[d8 + 1];
	}

	return buffer;
}

static char* i64toa(int64_t value, char* buffer) {
	uint64_t u = static_cast<uint64_t>(value);
	if (value < 0) {
		*buffer++ = '-';
		u = ~u + 1;
	}

	return u64toa(u, buffer);
}


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
	if (buff != fix_buff)
	{
		delete[] buff;
	}
}

Measure& Measure::Tag(const char* name, const char* value)
{
	if (tagcount >= MetricsTagMaxCount)
	{
		return *this;
	}
	tag_value[tagcount][0] = curpos;
	Write(name, strlen(name));
	tag_value[tagcount][1] = curpos;
	Write(value, strlen(value));
	tagcount++;
	return *this;
}

Measure& Measure::Tag(const char* name, const std::string& value)
{
	if (tagcount >= MetricsTagMaxCount)
	{
		return *this;
	}
	tag_value[tagcount][0] = curpos;
	Write(name, strlen(name));
	tag_value[tagcount][1] = curpos;
	Write(value.data(), value.length());
	tagcount++;
	return *this;
}

Measure& Measure::Tag(const char* name, int value)
{
	if (tagcount >= MetricsTagMaxCount)
	{
		return *this;
	}
	tag_value[tagcount][0] = curpos;
	Write(name, strlen(name));
	tag_value[tagcount][1] = curpos;
	Write(value);
	tagcount++;
	return *this;
}

Measure& Measure::Tag(const char* name, int64_t value)
{
	if (tagcount >= MetricsTagMaxCount)
	{
		return *this;
	}
	tag_value[tagcount][0] = curpos;
	Write(name, strlen(name));
	tag_value[tagcount][1] = curpos;
	Write(value);
	tagcount++;
	return *this;
}

Measure& Measure::Tag(const std::string& name, const char* value)
{
	if (tagcount >= MetricsTagMaxCount)
	{
		return *this;
	}
	tag_value[tagcount][0] = curpos;
	Write(name.data(), name.length());
	tag_value[tagcount][1] = curpos;
	Write(value, strlen(value));
	tagcount++;
	return *this;
}

Measure& Measure::Tag(const std::string& name, const std::string& value)
{
	if (tagcount >= MetricsTagMaxCount)
	{
		return *this;
	}
	tag_value[tagcount][0] = curpos;
	Write(name.data(), name.length());
	tag_value[tagcount][1] = curpos;
	Write(value.data(), value.length());
	tagcount++;
	return *this;
}

Measure& Measure::Tag(const std::string& name, int value)
{
	if (tagcount >= MetricsTagMaxCount)
	{
		return *this;
	}
	tag_value[tagcount][0] = curpos;
	Write(name.data(), name.length());
	tag_value[tagcount][1] = curpos;
	Write(value);
	tagcount++;
	return *this;
}

Measure& Measure::Tag(const std::string& name, int64_t value)
{
	if (tagcount >= MetricsTagMaxCount)
	{
		return *this;
	}
	tag_value[tagcount][0] = curpos;
	Write(name.data(), name.length());
	tag_value[tagcount][1] = curpos;
	Write(value);
	tagcount++;
	return *this;
}

MetricsData* Measure::Reg()
{
	MetricsData* p = g_metricsrecord.Reg(*this);
	return p;
}

MetricsData* Measure::Add(int64_t v)
{
	MetricsData* p = g_metricsrecord.Reg(*this);
	if (p)
	{
		p->Add(v);
	}
	return p;
}

MetricsData* Measure::Set(int64_t v)
{
	MetricsData* p = g_metricsrecord.Reg(*this);
	if (p)
	{
		p->Set(v);
	}
	return p;
}

MetricsData* Measure::Max(int64_t v)
{
	MetricsData* p = g_metricsrecord.Reg(*this);
	if (p)
	{
		p->Max(v);
	}
	return p;
}

std::string Measure::Snapshot(int64_t v, SnapshotType type, const std::string& metricsprefix, const std::map<std::string, std::string>& tags)
{
	if (curpos == 0)
	{
		return "";
	}
	std::string name;
	std::map<std::string, std::string> thistags;
	NameTagValue(name, thistags);

	std::ostringstream ss;
	if (type == Json)
	{
		ss << "\"metrics\":\"" << metricsprefix << name << "\",";
		for (const auto& t : thistags)
		{
			ss << "\"" << t.first << "\":\"" << t.second << "\",";
		}
		for (const auto& t : tags)
		{
			ss << "\"" << t.first << "\":\"" << t.second << "\",";
		}
		ss << "\"value\":" << v << "";
		ss << "}";
	}
	else if (type == Influx)
	{
		ss << metricsprefix << name;
		for (const auto& t : thistags)
		{
			ss << "," << t.first << "=" << t.second;
		}
		for (const auto& t : tags)
		{
			ss << "," << t.first << "=" << t.second;
		}
		ss << " value=" << v << "i\n";
	}
	else if (type == Prometheus)
	{
		ss << metricsprefix << name;
		if (!thistags.empty() || !tags.empty())
		{
			ss << "{";
			int index = 0;
			for (const auto& t : thistags)
			{
				ss << ((++index) == 1 ? "" : ",");
				ss << t.first << "=\"" << t.second << "\"";
			}
			for (const auto& t : tags)
			{
				ss << ((++index) == 1 ? "" : ",");
				ss << t.first << "=\"" << t.second << "\"";
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
	
	ExtraReserve(len);
	memcpy(buff + curpos, data, len);
	curpos += len;
}

void Measure::Write(int data)
{
	ExtraReserve(12);
	char* pbegin = &buff[curpos];
	char* pend = i32toa(data, pbegin);
	curpos += pend - pbegin;
}

void Measure::Write(int64_t data)
{
	ExtraReserve(24);
	char* pbegin = &buff[curpos];
	char* pend = i64toa(data, pbegin);
	curpos += pend - pbegin;
}

void Measure::ExtraReserve(int len)
{
	// 判断buff长度是否充足
	if ((curpos + len) > size - 1) // 这里-1用来控制最后一个字符为\0 直接可以当做字符串来使用
	{
		const static int buff_size = 128;
		size = buff_size*((curpos + len) / buff_size + 1);
		char* p = new char[size];
		memset(p, 0, size);
		memcpy(p, buff, curpos);

		if (buff != fix_buff)
		{
			delete[] buff;
		}
		buff = p;
	}
}

void Measure::NameTagValue(std::string& name, std::map<std::string, std::string>& tags) const
{
	if (tagcount == 0)
	{
		name = std::string(buff, curpos);
	}
	else
	{
		name = std::string(buff, tag_value[0][0]);
		for (int i = 0; i < tagcount; ++i)
		{
			int tag_begin = tag_value[i][0];
			int tag_end = tag_value[i][1];
			int value_begin = tag_value[i][1];
			int value_end = 0;
			if (i == tagcount - 1)
			{
				value_end = curpos;
			}
			else
			{
				value_end = tag_value[i + 1][0];
			}
			tags[std::string(&buff[tag_begin], &buff[tag_end])] = std::string(&buff[value_begin], &buff[value_end]);
		}
	}
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

MetricsRecord g_metricsrecord;

MetricsData* MetricsRecord::Reg(const Measure& measure)
{
	if (!brecord)
	{
		return NULL;
	}

	MetricsKey key;
	key.hash = BKDRHash(measure.buff);
	key.hash2 = APHash(measure.buff);

	// 先用共享锁 如果存在直接返回
	{
		READ_LOCK(mutex);
		auto it = ((const MetricsMap&)records).find(key); // 显示的调用const的find
		if (it != records.cend())
		{
			return it->second;
		}
	}

	// 不存在构造一个
	if (measure.curpos == 0)
	{
		return NULL;
	}
	std::string name;
	std::map<std::string, std::string> tags;
	measure.NameTagValue(name, tags);

	MetricsData* p = new MetricsData(name, tags);

	// 使用写锁
	{
		WRITE_LOCK(mutex);
		records.insert(std::make_pair(key, p));
	}
	return p;
}

std::string MetricsRecord::Snapshot(Measure::SnapshotType type, const std::string& metricsprefix, const std::map<std::string, std::string>& tags)
{
	MetricsMap lastdata;
	{
		READ_LOCK(mutex);
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
			ss << "\"metrics\":\"" << metricsprefix << it.second->name << "\",";
			for (const auto& t : it.second->tags)
			{
				ss << "\"" << t.first << "\":\"" << t.second << "\",";
			}
			for (const auto& t : tags)
			{
				ss << "\"" << t.first << "\":\"" << t.second << "\",";
			}
			ss << "\"value\":" << it.second->value << "";
			ss << "}";
		}
		ss << "]";
	}
	else if (type == Measure::Influx)
	{
		for (const auto& it : lastdata)
		{
			ss << metricsprefix << it.second->name;
			for (const auto& t : it.second->tags)
			{
				ss << "," << t.first << "=" << t.second;
			}
			for (const auto& t : tags)
			{
				ss << "," << t.first << "=" << t.second;
			}
			ss << " value=" << it.second->value << "i\n";
		}
	}
	else if (type == Measure::Prometheus)
	{
		for (const auto& it : lastdata)
		{
			ss << metricsprefix << it.second->name;
			if (!it.second->tags.empty() || !tags.empty())
			{
				ss << "{";
				int index = 0;
				for (const auto& t : it.second->tags)
				{
					ss << ((++index) == 1 ? "" : ",");
					ss << t.first << "=\"" << t.second << "\"";
				}
				for (const auto& t : tags)
				{
					ss << ((++index) == 1 ? "" : ",");
					ss << t.first << "=\"" << t.second << "\"";
				}
				ss << "}";
			}
			ss << " " << it.second->value << "\n";
		}
	}
	return ss.str();
}
