# MyCS — 多人联机射击游戏 Demo 实现文档

## 一、需求完成度评估

### 要求1：会移动和攻击玩家的敌人，玩家可以击败敌人
✅ **完成**

| 功能 | 实现文件 | 说明 |
|------|---------|------|
| NPC 移动/寻路 | `ShooterNPC` + `ShooterAIController` | StateTree 驱动 AI，导航网格寻路 |
| NPC 攻击玩家 | `ShooterNPC::StartShooting()` | AI感知系统检测玩家，锁定射击 |
| 玩家击败敌人 | `ShooterCharacter::TakeDamage()` | 投射物命中造成伤害，HP归零敌人死亡 |
| 武器系统 | `ShooterWeapon` / `ShooterProjectile` / `ShooterPickup` | 手枪/步枪/榴弹发射器，弹药消耗 |
| 手动换弹 | `ShooterWeapon::StartReload()` | R 键换弹，换弹期间不能射击 |

### 要求2：得分和游戏胜利机制
✅ **完成**

| 机制 | 实现 | 细节 |
|------|------|------|
| 游戏规则 | `ShooterGameMode` | 每15秒生成1只敌人，总计20只 |
| 胜利条件 | `HandleVictory()` | 消灭全部20只敌人 |
| 失败条件 | `HandleDefeat()` | 所有玩家命数耗尽 |
| 命数系统 | `MultiShooterPlayerState` | 每人3条命，死亡后消耗1条 |
| 击杀计分 | `AddKill()` / `AddDeath()` | 每杀一只+100分 |
| 血量恢复 | `OnHealthRegen()` | 每10秒恢复50HP，上限500 |

### 要求3：多人网络对战
✅ **完成**

| 功能 | 实现 | 细节 |
|------|------|------|
| 对象复制 | `bReplicates = true` | Character/NPC/Weapon/Projectile 均支持复制 |
| 服务器RPC | `ServerStartFiring()` / `ServerReload()` | 客户端射击/换弹请求发送到服务器 |
| 状态同步 | `DOREPLIFETIME` | HP/PendingKills/Score 等属性网络同步 |
| 游戏状态 | `MultiShooterGameState` | 总敌人数、存活敌人数、游戏结束标志 |
| 玩家状态 | `MultiShooterPlayerState` | 击杀数、死亡数、剩余命数 |
| 联机模式 | `OnlineSubsystemNull` | LAN 联机，支持 4 人同玩 |
| 角色碰撞 | `ShooterProjectile` | 友军伤害开启，玩家可互伤 |

---

## 二、整体架构

```
Source/MyCS/
├── MyCSCharacter.h/.cpp          # 基础FPS角色（相机、移动、跳跃）
├── MyCSGameMode.h/.cpp           # 基础游戏模式
├── MyCSPlayerController.h/.cpp   # 基础玩家控制器
├── MyCSCameraManager.h/.cpp      # 摄像机管理器
│
├── Variant_Shooter/              # ← 核心玩法模块
│   ├── ShooterCharacter.h/.cpp   # 玩家角色（HP/武器/复活/瞄准）
│   ├── ShooterGameMode.h/.cpp    # 游戏模式（刷怪/胜负判定）
│   ├── ShooterPlayerController.h/.cpp  # 玩家控制器（输入/重生）
│   │
│   ├── Weapons/
│   │   ├── ShooterWeapon.h/.cpp      # 武器基类（射击/弹匣/换弹）
│   │   ├── ShooterProjectile.h/.cpp  # 投射物（伤害/碰撞）
│   │   ├── ShooterPickup.h/.cpp      # 武器拾取物
│   │   └── ShooterWeaponHolder.h     # 武器持有者接口
│   │
│   ├── AI/
│   │   ├── ShooterNPC.h/.cpp         # 敌人NPC（血/伤害/死亡/射击）
│   │   ├── ShooterAIController.h/.cpp    # AI控制器（StateTree+感知）
│   │   ├── ShooterNPCSpawner.h/.cpp      # NPC生成器（旧系统）
│   │   ├── ShooterStateTreeUtility.h/.cpp # StateTree任务节点
│   │   └── EnvQueryContext_Target.h/.cpp  # 环境查询上下文
│   │
│   └── UI/
│       ├── ShooterUI.h/.cpp            # 计分板（废弃）
│       └── ShooterBulletCounterUI.h/.cpp # 弹夹UI（废弃）
│
├── Variant_Multi/                # ← 联机组网模块
│   ├── MultiShooterGameState.h/.cpp    # 游戏状态（网络同步）
│   ├── MultiShooterPlayerState.h/.cpp  # 玩家状态（击杀/命数）
│   └── MultiShooterGameHUD.h/.cpp      # 游戏HUD（绘制渲染）
│
└── Variant_Horror/               # Horror变体（未使用）
```

---

## 三、实现流程详解

### 阶段1：项目初始化

1. 基于 UE5.7 FPS 模板创建项目 `MyCS`
2. 模板自带内容：
   - `MyCSCharacter` — 第一人称角色（行走、跳跃、视角）
   - `MyCSPlayerController` — 输入映射管理
   - `MyCSGameMode` — 基础游戏模式
   - `MyCSCameraManager` — 摄像机角度限制

### 阶段2：武器系统

**ShooterWeaponHolder 接口** (`Weapons/ShooterWeaponHolder.h`)
- 定义武器持有者的通用契约
- 方法：`AttachWeaponMeshes()`、`PlayFiringMontage()`、`AddWeaponRecoil()`、`UpdateWeaponHUD()`、`GetWeaponTargetLocation()`、`AddWeaponClass()`、`OnWeaponActivated()`、`OnWeaponDeactivated()`、`OnSemiWeaponRefire()`

**ShooterWeapon** (`Weapons/ShooterWeapon.h/.cpp`)
- 第一/三人称网格双模渲染
- `MagazineSize` 弹匣容量、`CurrentBullets` 当前弹药
- 全自动/半自动射击模式 (`bFullAuto` + `RefireRate`)
- 弹道散布 (`AimVariance`)、后坐力 (`FiringRecoil`)
- AI 感知噪声: `MakeNoise()`
- **换弹系统**: `StartReload()` → `ReloadTimer` → `FinishReload()`，使用独立定时器避免与射击冲突

**ShooterProjectile** (`Weapons/ShooterProjectile.h/.cpp`)
- `SphereComponent` 碰撞 + `ProjectileMovementComponent` 弹道
- `HitDamage` 伤害值、`bExplodeOnHit` 爆炸模式
- 碰撞判定 → `ProcessHit()` → `ApplyDamage()`
- **友军伤害**: 不忽略其他玩家；NPC之间不互伤

**ShooterPickup** (`Weapons/ShooterPickup.h/.cpp`)
- 基于 `DataTable` 配置武器
- 碰撞重叠 → `AddWeaponClass()` → 隐藏+重生计时器

### 阶段3：玩家角色

**ShooterCharacter** (`ShooterCharacter.h/.cpp`)
继承自 `MyCSCharacter` + 实现 `IShooterWeaponHolder`
- `MaxHP = 500` 血量 → `TakeDamage()` → `Die()`
- `CurrentHP` 网络复制 (`ReplicatedUsing = OnRep_CurrentHP`)
- 武器管理: `OwnedWeapons[]` 武器库、`CurrentWeapon` 当前武器
- **射击**: `DoStartFiring()` → 客户端/服务器RPC → `Weapon->StartFiring()`
- **换弹**: `DoReload()` → 服务器RPC → `Weapon->StartReload()`
- **瞄准(ADS)**: `DoStartAim()` → `bIsAiming` → `Tick()` 中 FOV 插值: 70°→35°
- **血量恢复**: `OnHealthRegen()` 每10秒+50HP
- **死亡**: `Die()` → 武器停用 → GameMode通知 → 重生计时器
- 修改 `AddWeaponClass()` 支持已持有武器补弹

### 阶段4：敌人AI

**ShooterNPC** (`AI/ShooterNPC.h/.cpp`)
- 继承 `MyCSCharacter` + `IShooterWeaponHolder`
- `CurrentHP` 血量、`Die()` 布娃娃死亡 + 延迟销毁
- **击杀计数**: `LastDamageInstigator` 只记录玩家控制器

**ShooterAIController** (`AI/ShooterAIController.h/.cpp`)
- `StateTreeAIComponent` 行为树组件
- `AIPerceptionComponent` 感知组件（视觉+听觉）
- `OnPerceptionUpdated` → StateTree 任务

**StateTree 任务** (`AI/ShooterStateTreeUtility.h/.cpp`)
- `FStateTreeSenseEnemiesTask` — 感知敌人（基于Tag过滤）
- `FStateTreeShootAtTargetTask` — 射击目标
- `FStateTreeLineOfSightToTargetCondition` — 视线检测条件

### 阶段5：游戏模式与联机

**MultiShooterGameState** (`Variant_Multi/MultiShooterGameState.h/.cpp`)
- 网络同步字段: `MaxEnemies`、`TotalSpawned`、`TotalKilled`、`EnemiesRemaining`、`bGameOver`、`bVictory`
- 客户端通过读取 GameState 更新 HUD

**MultiShooterPlayerState** (`Variant_Multi/MultiShooterPlayerState.h/.cpp`)
- 网络同步字段: `Kills`、`Deaths`、`RemainingLives`
- `AddKill()` +=1 并更新 Score

**ShooterGameMode** (`ShooterGameMode.h/.cpp`)
- 默认 GameState/PlayerState/HUD 类设置
- 刷怪逻辑: `StartNextWave()` → `SpawnWaveEnemies()` → 每次生成一只
- 击杀计数: `OnEnemyKilled()` → `TotalKilled++`
- 玩家死亡: `OnPlayerDied()` → `RemainingLives--`
- 胜负判定: `HandleVictory()` / `HandleDefeat()`

**ShooterPlayerController** (`ShooterPlayerController.h/.cpp`)
- 输入映射管理（IMC）
- `OnPawnDestroyed()` → 检查命数 → 重生或淘汰
- 不再依赖 Blueprint UI Widget，全部交由 C++ HUD

### 阶段6：HUD系统

**MultiShooterGameHUD** (`Variant_Multi/MultiShooterGameHUD.h/.cpp`)
纯 C++ AHUD 子类，Canvas 绘制，无 Blueprint 依赖
- 左上角敌人信息面板: `Enemies: 15/20  Killed: 5`
- 左上角玩家信息面板: `Kills: 3  Score: 300` / `Lives: 3  Deaths: 1`
- 右下角弹药数: `12 / 30` 或红色 `RELOADING...`
- 空弹提示: `[ R ] RELOAD` 红色提示条
- 右下角血条: 绿/橙/红渐变，显示 `HP: 450 / 500`
- 屏幕中心十字准星: 白色十字 + 中心点
- 开镜瞄准镜: 程序化生成纹理，黑色背景+透明圆形镜片+绿色准线
- 游戏结束遮罩: 半透明全屏 + 大号 VICTORY/GAME OVER

### 阶段7：网络配置

**MyCS.uproject** 启用插件:
- `OnlineSubsystem` + `OnlineSubsystemNull` — LAN联机

**DefaultEngine.ini**:
- `[/Script/Engine.GameSession]` `MaxPlayers=4`
- `[/Script/OnlineSubsystem]` `DefaultPlatformService=Null`
- `[OnlineSubsystemNull]` `bIsLANMatch=True`

**MyCS.Build.cs**:
- 依赖模块: `NavigationSystem`、`UMG`、`Slate`、`AIModule`、`StateTreeModule`

---

## 四、核心数据流

### 射击流程
```
玩家按鼠标左键
  → DoStartFiring() [客户端]
    → ServerStartFiring() [RPC → 服务器]
      → Weapon->StartFiring()
        → Fire() → FireProjectile()
          → 生成 AShooterProjectile
            → NotifyHit() → ProcessHit() → ApplyDamage()
              → TakeDamage() (NPC或玩家)
```

### 刷新流程
```
GameMode::BeginPlay()
  → 定时器 5秒 → StartNextWave()
    → SpawnWaveEnemies()
      → 循环生成 NPC（每15秒1只）
        → NPC->OnPawnDeath 绑定 GameMode::OnEnemyKilled
    → 20只生成完毕后停止定时器
      → NPC死亡触发 OnEnemyKilled
        → TotalKilled++ → 检查胜利条件
```

### 网络同步
```
服务器端                           客户端
ShooterCharacter::CurrentHP       OnRep_CurrentHP → 更新 HUD
MultiShooterGameState::bGameOver  → 复制 → HUD 显示 GameOver
MultiShooterPlayerState::Kills    → 复制 → 左上角更新
```

---

## 五、操作说明

| 操作 | 键位 | 说明 |
|------|------|------|
| 移动 | WASD | 前后左右 |
| 跳跃 | 空格 | |
| 视角 | 鼠标移动 | |
| 射击 | 鼠标左键 | 自动/半自动取决于武器 |
| 瞄准 | 鼠标右键 | FOV缩放+瞄准镜 |
| 换弹 | R | 手动换弹 |
| 切枪 | Q 或 滚轮 | 在已持有的武器间切换 |
| 控制台 | ~ | 输入命令 |
| 联机(主机) | `open Lvl_Shooter?listen` | |
| 联机(加入) | `open <服务器IP>:7777` | |

---

## 六、游戏规则

- **敌人**: 每15秒生成1只，总数20只
- **胜利**: 消灭全部20只敌人
- **失败**: 所有玩家的命数耗尽
- **命数**: 每人3条命，死亡扣1条，归零后淘汰
- **血量**: 500HP，每10秒恢复50
- **武器**: 通过地图上的 Weapon Pickup 拾取（手枪/步枪/榴弹发射器）
- **友军伤害**: 开启，玩家可以攻击玩家
- **NPC互伤**: 关闭，NPC子弹不会伤害其他NPC
