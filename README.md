# BYTools

Qt/QML + C++ PC 工具集合应用骨架。

## 当前功能

- 主界面展示功能入口。
- **繁简转换**：批量将指定根目录下的文件名、文件夹名，或二者一起，从繁体中文转换为简体中文。
- **批量重命名**：批量重命名文件，支持多种命名规则（指定名称、查找替换），支持按文件类型筛选，支持递归处理子目录。
- **图片格式转换**：批量将图片转换为指定格式（PNG、JPG、BMP、WebP、TIFF），支持递归子文件夹和质量调节。
- **图片裁剪**：按比例或指定像素尺寸裁剪图片，支持实时预览、拖拽调整裁剪框、多图浏览切换，支持覆盖源文件或输出到新目录。
- **视频字幕翻译**：从视频中提取音频，使用 Whisper 进行语音识别生成SRT字幕，支持翻译SRT字幕（百度翻译API/本地翻译），内嵌SRT字幕到视频（支持 GPU 加速）。

## 目录结构

```text
src/
  main.cpp                     程序入口
  app/                         应用控制器
  core/                        核心模块（插件管理器、接口定义）
plugins/                       插件目录
  nameconverter/               繁简转换插件
  batchrename/                 批量重命名插件
  imageconverter/              图片格式转换插件
  imagecrop/                   图片裁剪插件
  videosubtitle/               视频字幕翻译插件
qml/
  components/                  可复用界面组件
  pages/                       页面
third/
  ffmpeg					   可用的ffmpeg
  model						   模型存放目录（需手动下载）
  whisper_amd				   支持AMD显卡的whisper程序
  whisper_cpu				   官网下载的whisper程序，只支持CPU，速度较慢
resources/                     图标、资源文件
```

## 插件架构

项目采用插件化架构，每个功能作为独立插件实现：

1. 插件需实现 `PluginInterface` 接口
2. 插件编译后自动生成到 `build/plugins/<plugin-name>` 目录
3. 应用启动时自动加载所有插件

## 新增插件

添加新功能插件步骤：

1. 在 `plugins/` 目录下创建新插件目录
2. 创建插件类实现 `PluginInterface` 接口
3. 创建控制器类处理业务逻辑
4. 在 `qml/pages/` 创建对应的 QML 页面
5. 配置 CMakeLists.txt

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

## 运行

构建完成后，直接运行 `build/BYTools.exe` 即可，插件会自动加载。