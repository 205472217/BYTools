# BYTools

Qt/QML + C++ PC 工具集合应用骨架。

## 当前功能

- 主界面展示功能入口。
- 批量将指定根目录下的文件名、文件夹名，或二者一起，从繁体中文转换为简体中文。
- 支持预览后执行，减少误操作。

## 目录结构

```text
src/
  app/                         应用级控制器与功能注册
  core/                        通用数据结构、结果类型
  features/
    renameconverter/           文件/文件夹名称繁简转换功能
qml/
  components/                  可复用界面组件
  pages/                       页面
resources/                     后续图标、资源文件
```

## 后续扩展建议

新增功能时，在 `src/features/<feature-name>` 放 C++ 业务代码，在 `qml/pages` 放页面，再通过 `AppController` 注册功能入口。

计划中的浏览器操作、Windows 定时任务功能，也可以按独立 feature 模块接入。

## 构建

需要 Qt 6.5+ 与 CMake。

当前机器已验证可用的构建方式：

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
```

通用方式：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

如果使用 Qt Creator，建议选择与 Qt 安装版本匹配的 MinGW kit。不要混用 Visual Studio 编译器和 MinGW 版 Qt。
