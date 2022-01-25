# metrics
指标数据统计支持，支持influxdb、prometheus格式的数据快照

# 使用案例
## 将数据添加到记录中，使用 g_metricsrecord.Snapshot(Measure::Prometheus) 获取格式化的快照数据
```
Measure("metricsname").Tag("tag", 12).Add(1);
Measure("metricsname").Tag("tag", "id").Set(10);
Measure("metricsname").Tag("tag", "max").Max(10);

下面这个中方式效率非常高，可以考虑在热点地方使用，提高性能，要求metricsname和tags都是固定的
Metrics* pMetrics = Measure("metricsname").Tag("tag", "name").Reg();		// pMetrics对象保存下来，在其他地方使用
static Metrics* pMetrics = Measure("metricsname").Tag("tag", "name").Reg();	// 用静态的方式保存pMetrics
pMetrics->Add(1);

```
## 直接获取格式化的快照数据
```
std::string str = Measure("metricsname").Tag("tag", 12).Snapshot(1, Measure::Prometheus);
std::string str = Measure("metricsname").Tag("tag", "id").Snapshot(10, Measure::Prometheus);
std::string str = Measure("metricsname").Tag("tag", "max").Snapshot(10, Measure::Prometheus);
```
