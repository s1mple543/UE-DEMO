# MyCS — 多人联机射击游戏 面试项目介绍

## 项目概述

| 项目 | 说明 |
|------|------|
| **项目名称** | MyCS — 多人波次生存射击游戏 (Multiplayer Wave Survival Shooter) |
| **引擎版本** | Unreal Engine 5.7 |
| **开发语言** | C++ (纯代码，不依赖蓝图逻辑) |
| **开发周期** | 约 2 周 |
| **项目规模** | 50+ 源文件，约 6000 行 C++ 代码 |
| **核心玩法** | 4人联机合作，每15秒生成1只敌人，总计20只，全清获胜 |

---

## 一、整体架构

项目基于 UE5 First Person 模板扩展，采用 **分层架构**：

```
Source/MyCS/
├── MyCS/                    # 基础层（FPS模板基类）
│   ├── MyCSCharacter        # 角色基础（移动/跳跃/相机）
│   ├── MyCSPlayerController # 控制器基础
│   ├── MyCSGameMode         # 游戏模式基础
│   └── MyCSCameraManager    # 相机约束
│
├── Variant_Shooter/         # 玩法层（核心游戏逻辑）
│   ├── ShooterCharacter     # 玩家角色（HP/武器/换弹/瞄准）
│   ├── ShooterGameMode      # 游戏模式（刷怪/胜负判定）
│   ├── ShooterPlayerController # 控制器（输入/复活/菜单）
│   ├── Weapons/             # 武器系统
│   ├── AI/                  # 敌人AI（StateTree驱动）
│   └── UI/                  # 旧版UI（已废弃）
│
├── Variant_Multi/           # 网络层（联机同步）
│   ├── MultiShooterGameState    # 游戏状态同步
│   ├── MultiShooterPlayerState  # 玩家状态同步
│   └── MultiShooterGameHUD      # HUD渲染+菜单
│
└── Variant_Horror/          # 未使用
```

---

## 二、各模块详细说明（面试重点）

### 模块1：第一人称角色系统

#### 基类 MyCSCharacter

UE5 FPS 模板提供的基类，负责：

- **相机附着**：第一人称网格 + Camera 组件设置
- **输入系统**：WASD 移动、鼠标视角、空格跳跃
- **输入绑定**：Enhanced Input System 增强输入

**我们的修改**：

| 修改 | 文件 | 说明 |
|------|------|------|
| 修复穿模 | `MyCSCharacter.cpp` | 第一人称网格从附着于 TP 骨骼改为附着于 Camera，解决手臂穿墙问题 |
| 相机偏移调整 | `MyCSCharacter.cpp` | 调整相机位置至合理眼睛高度 (0, 0, 64)，手臂显示在视野下方 |

**面试要点**：可以阐述 UE5 第一人称系统的 SetupAttachment 机制，以及为什么 FP 网格附着在 Camera 上可以避免墙壁穿透——因为 Camera 的 Near Clip Plane 会裁掉网格，且网格不随身体动画摆动。

---

### 模块2：武器系统（面试重点）

#### ShooterWeapon 武器基类

完整的武器系统，支持：

- **双网格渲染**：第一人称网格（玩家可见，带手臂）+ 第三人称网格（其他人可见）
- **弹药系统**：`MagazineSize` 弹匣容量、`CurrentBullets` 当前弹药
- **射击模式**：全自动 (`bFullAuto`) / 半自动
- **弹道**：基于 `Projectile` 的投射物，带散布 (`AimVariance`) 和后坐力 (`FiringRecoil`)
- **AI感知**：射击时 `MakeNoise()` 触发 AI 听觉感知

**我们新增的换弹系统**：

```cpp
void StartReload();     // 开始换弹，标记 bIsReloading，设置换弹计时器
void FinishReload();    // 换弹完成，填满弹匣，更新HUD
```

**关键设计决策**：换弹使用 `ReloadTimer`（独立于射击的 `RefireTimer`），避免换弹和射击的定时器相互覆盖导致卡死。

**自动换弹删除**：原模板在 `FireProjectile()` 中子弹打空后自动填满弹匣，我们删除了这个行为，改为必须按 R 键手动换弹。

#### ShooterProjectile 投射物

- **碰撞检测**：`SphereComponent` + `ProjectileMovementComponent`
- **伤害系统**：`HitDamage` 伤害值，通过 `ApplyDamage()` 统一接口
- **爆炸模式**：`bExplodeOnHit` 开启时使用 `OverlapMultiByObjectType` 检测范围伤害

**我们的关键修改**：

```cpp
// NPC vs NPC 不造成伤害
bool bInstigatorIsPlayer = InstigatorPawn->IsPlayerControlled();
bool bTargetIsPlayer = HitCharacter->IsPlayerControlled();
if (bInstigatorIsPlayer || bTargetIsPlayer)
    ApplyDamage(...)
```

**设计考量**：
- 友军伤害开启（玩家可互伤）→ 增加策略性
- NPC 自相残杀关闭 → 防止敌人内斗，保证游戏难度

#### ShooterPickup 武器拾取

- 基于 `DataTable` 配置武器类型（Pistol/Rifle/GrenadeLauncher）
- 碰撞重叠触发 `AddWeaponClass()` 获取武器
- 拾取后隐藏 + 定时器重生

---

### 模块3：玩家角色（面试重点）

#### ShooterCharacter

继承自 `MyCSCharacter`，实现 `IShooterWeaponHolder` 接口。

**状态管理**：

| 状态 | 类型 | 同步方式 |
|------|------|---------|
| HP | `float CurrentHP` | `ReplicatedUsing=OnRep_CurrentHP` |
| 最大血量 | `float MaxHP = 500` | 仅服务器 |
| 死亡标记 | `FName DeathTag = "Dead"` | Tag |
| 瞄准状态 | `bool bIsAiming` | 本地 |

**核心功能**：

1. **伤害与死亡**

```cpp
float TakeDamage(float Damage, ...) {
    CurrentHP -= Damage;
    if (CurrentHP <= 0) Die();
    OnDamaged.Broadcast(HP/MaxHP);  // 更新HUD
}
void Die() {
    CurrentWeapon->DeactivateWeapon();
    GameMode->OnPlayerDied(GetController());  // 通知GameMode扣命
    DisableInput(nullptr);                    // 禁用操作
    ScheduleRespawn();                        // 5秒后重生
}
```

2. **射击系统**（支持网络同步）

```cpp
void DoStartFiring() {
    if (HasAuthority()) CurrentWeapon->StartFiring();
    else ServerStartFiring();  // RPC → 服务器
}
```

3. **血量恢复系统**

```cpp
void OnHealthRegen() {
    if (IsDead() || !HasAuthority()) return;
    CurrentHP = FMath::Min(MaxHP, CurrentHP + 50.0f);
}
```

每10秒恢复50HP，仅服务器执行，`OnRep_CurrentHP` 自动同步到客户端。

4. **瞄准系统（ADS）**

```cpp
void Tick(float DeltaTime) {
    float Target = bIsAiming ? AimFOV : DefaultFOV;  // 35° vs 70°
    float NewFOV = FMath::FInterpTo(Current, Target, Delta, 12.0f);
    Camera->FieldOfView = NewFOV;  // 平滑过渡
}
```

---

### 模块4：敌人 AI（面试重点）

#### ShooterNPC

继承自 `MyCSCharacter` + `IShooterWeaponHolder`。

**AI 架构**：StateTree 行为树

```
StateTree 流程:
  SenseEnemies → 感知范围内检测玩家
    ↓ 发现敌人
  LineOfSightToTarget → 视线检测
    ↓ 有视线
  FaceActor → 面向目标
  ShootAtTarget → 开火
    ↓ 丢失目标
  Roam → 随机巡逻
```

**击杀逻辑**：

```cpp
void Die() {
    if (bIsDead) return;
    bIsDead = true;
    OnPawnDeath.Broadcast();  // 触发 GameMode 击杀计数
    if (LastDamageInstigator) // 记录击杀者
        PS->AddKill();
    // 布娃娃物理死亡
    GetMesh()->SetSimulatePhysics(true);
}
```

**友军伤害防护**：`TakeDamage` 只记录 `IsPlayerControlled()` 的击杀者，排除 NPC 误伤。

---

### 模块5：游戏模式（面试重点）

#### ShooterGameMode

游戏规则的核心控制器。

**刷怪逻辑**：每个敌人都在服务器端生成 (`HasAuthority`)，客户端不会自主生成 NPC。

**关键数据结构**：

```cpp
int32 TotalSpawned = 0;   // 已生成数量
int32 TotalKilled = 0;    // 已击杀数量
bool bFinishedSpawning;   // 是否已生成完毕
bool bGameOver;           // 游戏是否结束
```

**胜负判定**：

```cpp
// 胜利：所有敌人生成完毕且全部击杀
if (bFinishedSpawning && TotalKilled >= MaxEnemies)
    HandleVictory();

// 失败：所有玩家命数为0
void CheckDefeat() {
    for (PlayerState : PlayerArray)
        if (MPS->RemainingLives > 0) return;  // 还有人活着
    HandleDefeat();
}
```

---

### 模块6：多人网络同步（面试重点）

#### 同步架构

本项目使用 **状态同步（State Synchronization）**，也叫 **服务端权威模式**。

#### MultiShooterGameState

服务器→客户端推送到所有玩家：

```cpp
DOREPLIFETIME(AMultiShooterGameState, MaxEnemies);
DOREPLIFETIME(AMultiShooterGameState, TotalSpawned);
DOREPLIFETIME(AMultiShooterGameState, TotalKilled);
DOREPLIFETIME(AMultiShooterGameState, EnemiesRemaining);
DOREPLIFETIME(AMultiShooterGameState, bGameOver);
DOREPLIFETIME(AMultiShooterGameState, bVictory);
```

#### MultiShooterPlayerState

服务器→指定客户端推送（每个玩家看到自己的数据）：

```cpp
DOREPLIFETIME(AMultiShooterPlayerState, Kills);
DOREPLIFETIME(AMultiShooterPlayerState, Deaths);
DOREPLIFETIME(AMultiShooterPlayerState, RemainingLives);
```

#### RPC 调用

| RPC | 方向 | 用途 |
|-----|------|------|
| `ServerStartFiring()` | 客户端→服务器 | 请求开枪 |
| `ServerStopFiring()` | 客户端→服务器 | 请求停止开枪 |
| `ServerReload()` | 客户端→服务器 | 请求换弹 |

#### 对象复制

所有游戏对象都开启了复制：

```cpp
bReplicates = true;  // Character、NPC、Weapon、Projectile、Pickup
```

**网络配置**：
- `OnlineSubsystemNull` LAN 联机（无需 Steam）
- `MaxPlayers=4` 最大 4 人
- 控制台命令：`open Lvl_Shooter?listen`（主机）/ `open IP:7777`（加入）

---

### 模块7：HUD系统

#### MultiShooterGameHUD

纯 C++ 的 AHUD 子类，用 Canvas 绘制，零蓝图依赖。

**绘制内容**：

| 元素 | 位置 | 技术实现 |
|------|------|---------|
| 敌人信息 | 左上角 | `DrawText()` + `FCanvasTileItem` 背景 |
| 玩家统计 | 左上角 | 击杀数、分数、命数、死亡数 |
| 弹药计数 | 右下角 | `12 / 30` 或 `RELOADING...` |
| 空弹提示 | 弹药下方 | 红色 `[ R ] RELOAD` |
| 血条 | 右下角 | 绿/橙/红 三段渐变色条 |
| 十字准星 | 屏幕中心 | 4条白色线段 + 中心点 |
| 瞄准镜 | 全屏 | 程序化生成纹理，圆形透明镜片+绿色准线 |
| 游戏结束 | 全屏 | 半透明遮罩 + VICTORY/GAME OVER |
| 主菜单 | 全屏 | F10呼出，Host/Join/Quit |

#### 菜单系统

- **F10** 呼出/关闭菜单，游戏暂停
- **HOST GAME** → 创建房间
- **JOIN GAME** → Localhost / LAN 快速加入
- **QUIT** → 退出游戏

---

### 模块8：地图生成

#### BuildMap.py

UE5 Python 脚本，在编辑器内一键生成竞技场地图。

**生成内容**：
- 120m×120m 平底竞技场
- 四边围墙（6米高）
- 4个仓库（四象限各一）
- 24个L型掩体墙
- 8个玩家出生点
- NavMesh 导航网格
- 灯光系统（太阳+天光+9个点光源）
- 20个武器拾取点

**技术特点**：使用 UE5 Python API 自动化级别设计。

---

## 三、面试可能问到的技术问题

### Q1：为什么要用状态同步而不是帧同步？

FPS 游戏对响应速度要求极高。状态同步允许**客户端预测**（Client-side Prediction）——玩家按 W 键时不等服务器确认就立刻移动，然后由服务器做校正。而帧同步必须等所有客户端确认输入才能推进，延迟直接表现为卡顿。UE5 的 `CharacterMovementComponent` 内置了客户端预测和服务器校正，所以我们天然使用的是状态同步模型。参考游戏：《堡垒之夜》《使命召唤》《绝地求生》。

### Q2：如何处理网络延迟下的射击判定？

我们使用的是服务端权威模式——客户端按下鼠标左键时调用 `ServerStartFiring()` RPC 通知服务器，服务器在服务端执行 `Fire()` 和 `Projectile` 的碰撞检测。这是最简单的方案。更精确的 FPS 会做 **服务器回滚（Server-side Rewind）**，即服务器把射击者的位置回溯到开枪那一刻来判定是否命中。

### Q3：换弹系统的设计考量？

换弹使用了独立的 `ReloadTimer`（FTimerHandle），与射击的 `RefireTimer` 分离。这样换弹过程中如果有火制定时器残留，两个定时器不会互相干扰。换弹期间 `bIsReloading` 标志为 true，`StartFiring()` 中检查此标志并拒绝开火，保证换弹过程的完整性。

### Q4：如何防止 NPC 自相残杀？

在 `ShooterProjectile::ProcessHit()` 中增加保护：如果**射击者和目标都不是玩家**（即两个 NPC 之间），跳过伤害。同时 NPC 的 `TakeDamage()` 只会记录 `IsPlayerControlled()` 的控制器作为击杀者，排除 NPC 互杀导致的虚假击杀计数。

### Q5：友军伤害的设计考虑？

保留友军伤害是为了增加游戏的策略深度和紧张感——玩家需要留意队友位置，不能无脑扫射。但 NPC 之间禁止互伤，因为这会降低游戏难度（敌人在内讧）。

---

## 四、数据流图

### 射击流程
```
玩家点击左键
  → DoStartFiring() (客户端)
    → ServerStartFiring() (RPC到服务器)
      → Weapon->StartFiring()
        → Fire() → FireProjectile()
          → SpawnActor<AShooterProjectile>
            → NotifyHit() → ProcessHit()
              → ApplyDamage() → TakeDamage()
```

### 刷怪流程
```
GameMode::BeginPlay()
  → 5秒后开始刷怪
    → 每15秒生成1只 (SetTimer循环)
      → NPC 绑定 OnPawnDeath → OnEnemyKilled
      → 20只生成完毕 → bFinishedSpawning = true
        → NPC死亡触发 OnEnemyKilled
          → TotalKilled++
          → 若 TotalKilled >= 20 → Victory
```

### 网络同步
```
服务器                           客户端
ShooterCharacter::CurrentHP ───→ OnRep_CurrentHP → HUD更新
MultiShooterGameState ─────────→ 自动复制 → 所有客户端读取
MultiShooterPlayerState ───────→ 自动复制 → 对应玩家读取
ServerStartFiring() ←────────── 客户端 RPC 请求
```

---

## 五、操作方式

| 操作 | 键位 | 说明 |
|------|------|------|
| 移动 | WASD | 前后左右 |
| 跳跃 | 空格 | |
| 视角 | 鼠标 | |
| 射击 | 鼠标左键 | |
| 开镜 | 鼠标右键 | FOV缩放+瞄准镜 |
| 换弹 | R | 手动换弹 |
| 主菜单 | F10 | Host/Join/Quit |
| 联机(主机) | F10→Host | 自动执行 open Lvl_Shooter?listen |
| 联机(加入) | F10→Join→Localhost/LAN | 自动连接 |

---

## 六、行为规范与技术亮点

### 代码质量规范
- **命名规范**: 使用 UE5 标准命名法（类名前缀、驼峰命名）
- **注释精简**: 只有 WHY 注释，不做 WHAT 注释
- **职责分离**: 每个类单一职责，不多做功能耦合
- **防御性编程**: `IsValid()` 检查指针、`FMath::Clamp/Max` 边界保护

### UE5 技术亮点
- **Enhanced Input System**: 行为驱动的输入系统，支持上下文映射
- **StateTree AI**: 基于状态树的可视化 AI 行为编排，替代传统行为树
- **Replication System**: UE5 的属性复制和 RPC 机制
- **Canvas HUD**: 零依赖的纯 C++ UI，手动绘制全部界面
- **Programmatic Texture**: C++ 中动态生成 UTexture2D 纹理（瞄准镜）
- **Python Scripting**: 使用 Python API 自动化编辑器操作（地图生成）
