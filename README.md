# UELogViewer 🎮

Notepad++ 插件，为虚幻引擎（Unreal Engine）日志文件提供语法高亮。

## ✨ 效果

根据日志级别自动染色：

| 级别 | 颜色 | 说明 |
|---|---|---|
| `Fatal` | 白字红底 | 致命错误，整行高亮 |
| `Error` | 🔴 红色 | 错误信息 |
| `Warning` | 🟡 黄色 | 警告信息 |
| `Display` | 🔵 青色 | 显示信息 |
| `Log` | ⚪ 浅灰 | 普通日志 |
| `Verbose` | 灰色 | 详细日志 |
| `VeryVerbose` | 深灰 | 非常详细日志 |
| 时间戳 `[...]` | 🟢 绿色 | 时间戳部分 |
| Category `LogXxx` | 🟣 紫色 | 日志分类 |

### 支持的日志格式

```
[2024.03.15-10.22.31:123][  0]LogInit: Display: Starting UE5...
[2024.03.15-10.22.31:456][  1]LogNet: Warning: Packet loss detected
[2024.03.15-10.22.32:789][  2]LogBlueprintUserMessages: Error: Accessed None
LogAI: Verbose: Pathfinding complete
```

## 🚀 安装

### 方式一：直接使用预编译 DLL（推荐）

1. 下载 `UELogViewer.dll`（位于 `build/bin/Release/` 目录）
2. 将 `UELogViewer.dll` 复制到 Notepad++ 插件目录：
   ```
   C:\Program Files\Notepad++\plugins\UELogViewer\UELogViewer.dll
   ```
3. 重启 Notepad++

### 方式二：自行编译

见下方 [构建](#-构建) 章节。

## 🎯 使用方法

1. 用 Notepad++ 打开 `.log` 文件
2. 菜单栏 → `插件` → `UELogViewer` → `Apply UE Log Highlighting`
3. 高亮立即生效，后续编辑实时更新

如需恢复默认样式：`插件` → `UELogViewer` → `Reset Highlighting`

## 🔧 构建

### 前置条件

- Visual Studio 2022（或 VS2022 Build Tools）
- Windows SDK 10.0+

### 编译步骤

```bat
git clone https://github.com/zhangchenglei2/UELogViewer.git
cd UELogViewer\build
msbuild UELogViewer.sln /p:Configuration=Release /p:Platform=x64
```

输出：`build\bin\Release\UELogViewer.dll`

也可以直接用 Visual Studio 打开 `build\UELogViewer.sln` 编译。

## 🗂️ 项目结构

```
UELogViewer/
├── src/
│   └── UELogViewer.cpp          # 插件主源码（高亮逻辑全在这里）
├── include/
│   ├── ScintillaDefs.h          # Scintilla 常量 & SCNotification 定义
│   ├── PluginInterface.h        # Notepad++ 插件接口
│   └── Notepad_plus_msgs.h      # NPP 消息常量
└── build/
    ├── UELogViewer.sln          # VS 解决方案
    ├── UELogViewer.vcxproj      # VS 项目文件
    └── bin/Release/
        └── UELogViewer.dll      # 编译产物
```

## 🛠️ 二次开发

高亮规则全在 `src/UELogViewer.cpp` 的两处位置：

**修改颜色** — 找到 `k_styles[]` 数组：
```cpp
{ STYLE_UE_ERROR, RGB_TO_SCINTILLA(255, 80, 80), 0, true, false },
//                                 R    G   B
```

**修改识别规则** — 找到 `parseLine()` 函数，修改 verbosity 关键词匹配逻辑。

改完重新编译替换 DLL 即可，无需重新安装。

## 📄 开源协议

MIT
