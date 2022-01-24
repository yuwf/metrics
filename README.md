# metrics
指标数据统计支持，支持influxdb、prometheus格式的数据快照

# 使用案例
## 将数据添加到记录中，使用 g_metricsrecord.Snapshot() 获取格式化的快照数据
```
Measure("metricsname").Tag("tag", 12).Add(1);
Measure("metricsname").Tag("tag", "id").Set(10);
Measure("metricsname").Tag("tag", "max").Max(10);
```
## 直接获取格式化的快照数据
```
std::string str = Measure("metricsname").Tag("tag", 12).Snapshot(1);
std::string str = Measure("metricsname").Tag("tag", "id").Snapshot(10);
std::string str = Measure("metricsname").Tag("tag", "max").Snapshot(10);
```
