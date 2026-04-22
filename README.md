# HFT (High-Frequency Trading) Engine 极速量化执行引擎基础版

这是一个基于 C++20 的高性能量化执行引擎本地模拟版本。
整个系统采用标准的 SPSC（Single-Producer Single-Consumer）无锁环形队列和内存对齐（`alignas(64)`）技术以避免 False Sharing（伪共享）。同时实现了将线程与 CPU 核心硬绑核操作（`pthread_setaffinity_np`）。

---

## 极致性能优化指南 (Linux 核心隔离配置)

为了追求真正的极速执行与超低延迟，需要将核心 2 和核心 3 从操作系统的通用调度器中完全隔离出来。这可以避免系统时钟中断、背景服务上下文切换以及软中断影响我们策略线程轮询的延迟。

### 1. 修改 GRUB 添加 `isolcpus` 参数

我们需要修改启动引导器：

1. 编辑默认的 GRUB 配置文件：
   ```bash
   sudo nano /etc/default/grub
   ```

2. 找到包含 `GRUB_CMDLINE_LINUX` 或者 `GRUB_CMDLINE_LINUX_DEFAULT` 的那一行。在引号内部追加 `isolcpus=2,3`。
   例如：
   ```text
   GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=2,3"
   ```
   > *(注意：现代内核还可能推荐搭配使用 `nohz_full=2,3` 以及 `rcu_nocbs=2,3` 来更彻底地关掉 Tick 中断)*

3. 更新 GRUB 配置并重启系统使其生效：
   - 对于 Debian/Ubuntu 系：
     ```bash
     sudo update-grub
     ```
   - 对于 RHEL/Fedora/CentOS 系系统：
     ```bash
     sudo grub2-mkconfig -o /boot/grub2/grub.cfg
     ```
   - 对于 Arch/Manjaro 等基于 systemd-boot 或者是传统 GRUB 路径稍有不同的系统：
     ```bash
     sudo grub-mkconfig -o /boot/grub/grub.cfg
     ```

4. 重启计算机：
   ```bash
   sudo reboot
   ```

5. 重启后检查内核参数是否生效：
   ```bash
   cat /proc/cmdline
   ```
   如果输出文字包含了 `isolcpus=2,3` 即表示核心已被成功隔离。

---

## 编译与运行该工程

本项目使用现代 CMake (最低版本要求 3.16) 和 C++20 标准。编译代码时 CMake 脚本已经默认配置了 `-O3` 极致优化级别和 `-march=native` 本地高级指令集支持。

### 编译步骤

1. 在项目根目录下生成并构建（Build）：
   ```bash
   # 生成构建配置，并将临时产物置于 build/ 下
   cmake -B build
   # 进行多线程并行编译
   cmake --build build -j $(nproc)
   ```

### 运行方式

编译完成后，由于定制了产出目录规则，二进制可执行文件默认放置于 `bin/` 文件夹下。

```bash
# 启动 HFT 基础组件
./bin/hft_engine
```

运行时，引擎会先验证其自身 `TickData`、`OrderSignal` 以及核心 SPSC 无锁队列的 64 字节对齐状况，随即在核心 2 上启动一条专门用于生成 Mock 数据的挂载线程，并在核心 3 上启动用于 `while(true)` Busy Polling 的策略接收线程以观测极速穿透延迟。
