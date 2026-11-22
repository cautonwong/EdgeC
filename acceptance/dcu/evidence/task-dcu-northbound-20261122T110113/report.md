结论: 未完成 —— 没有不合格也没有无效判定,但存在申报外未测或平台错误:不能判合格 · platform_source: package
任务 dcu-northbound · 产品 dcu · 筛选 tag:北向
通过率: 设备级 1/1 台 · 场景级 0/0 条(任务结论取两栏最差)
分母: select ∩ 适用矩阵 命中 20 条(不含被 --select / 适用矩阵筛掉的 3 条)

[不合格] 0 条(设备 id · 实测值 · 边界 · 超出多少)
  无

[场景归属] 0 条(设备 / 台架 / 平台,不合并成一句「场景失败」)
  无

[测试无效] 0 条(断链 / 台架故障:判定被环境吞掉,不是设备不合格)
  无

[未测·申报内(灰)] 0 条 —— 理由只用四类:平台不支持 / 设备缺能力 / 人工步骤 / 已知缺陷跳过
  无
[未测·申报外(红)] 20 条 —— 有一条,结论就不能是合格
  D_00_00_02_000001  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[0]
  D_00_00_02_000002  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[1]
  D_00_00_02_000003  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[2]
  D_00_00_02_000009  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[3]
  D_00_00_02_000004  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[4]
  D_00_00_02_000005  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[5]
  D_00_00_02_000006  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[6]
  D_00_00_02_000007  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[7]
  D_00_00_02_000008  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[8]
  D_00_00_02_000018  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[9]
  D_00_00_02_000019  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[10]
  D_00_00_02_000010  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[11]
  D_00_00_02_000011  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[12]
  D_00_00_02_000012  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[13]
  D_00_00_02_000013  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[14]
  D_00_00_02_000014  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[15]
  D_00_00_02_000015  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[16]
  D_00_00_02_000016  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[17]
  D_00_00_02_000020  理由不在申报的四类里:Skipped: 没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）  证据 run-scope.json#skipped[18]
  D_00_00_02_000017  pytest 报了失败,证据里没有结论:testplatform.transport.interfaces.IncompleteFrameError: TCP 127.0.0.1:19212: 2.0 秒内没收满一帧(收到 0 字节,整帧需要 未知 字节),半帧不交给上层:  证据 run-scope.json#failed[0]

[产品退出] 1 次
  产品退出(how=platform_killed, code=None, signal=15) —— 平台发起的终止,不产生结论

[平台错误] 0 条(聚合根不变量被违反:平台自身有 bug)
  无

