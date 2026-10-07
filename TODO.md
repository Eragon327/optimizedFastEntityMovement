# TODO

## 背景

- 26.x 的 `MoveCollisionSystem::fetchCollisionShapes` 在函数开头会把 `MoveRequestComponent::mSpeed`
  的模长压到 16 格/gt，实体无法再像 1.21.3 那样单 tick 移动 >16 格。
- 本 mod 的策略：hook `Actor::move`，当 `|posDelta| > 16` 时按 Y -> X -> Z 拆轴、
  每轴再按 <=16 分段，每段都跑一次完整的原版 ActorMove 管线，使扫掠盒始终很薄（O(N) 而非 O(N^3)），
  避免 `getFetchBoxSubtraction` 里 `reserve(getVolume(box))` 的巨量枚举导致的卡死/崩溃。
- 当前只对「非玩家、非生物」启用（见 `src/ofem/base/Hooks.cpp` 的 `shouldSplitMove`）。

## 调用链要点（1.21.3 逆向结论）

1. `MoveCollisionSystem::fetchCollisionShapes`（RVA `0xA70890`）由
   `MoveCollisionSystem::System::singleTick`（`0xA81F10`）里的 `ViewT<...>::on<lambda>`（`0xA60940`）调用；
   另一条路径是 `System::tick` -> `entt::view::each` -> `std::apply<lambda>`。
2. 扫掠盒本身是**局部** AABB，只有 `request+0x18`（`mMoveCollisionLastFetchedBox`）和
   `request+0x58`（`mCollisionShapes`）会逃出函数。
3. `getFetchBoxSubtraction`（`0xA7DAA0`）把盒子按 **1x1x1** 单元切分（step 1.0，对齐 box.min），
   只保留不被 `lastBox` 包含的格；崩溃面就在 `reserve((unsigned)getVolume(box))`。
4. 碰撞/轨迹判定在 `SweptMovement::computeMoveWithDepenetration`（`0xBAD6F0`，由
   `computeYXZMoveWithDepenetration` `0xBADC00` 组装 Y/X/Z 步后调用），底层是 `AABB::clipCollide`；
   真正落地（写回 `StateVectorComponent` / 碰撞标志）在 `FinalizeMoveSystemImpl::tickFinalizeMoveSystem`。
   执行 `ActorMove` 整条类别的是 `Actor::move` -> `EntitySystems::_singleTickCategory`。

## 已完成

- [x] 逐轴 + 逐段（<=16）拆分：`ActorMoveHook`
- [x] 类型过滤：改用引擎自带的 `ActorClassTree::isInstanceOf(actor, ActorType::Mob)`
      （1.21.3 `ACF::isInstanceOf` @ `0x4ECE00`，内部 `getEntityTypeId()` + 位运算：
      `low == type & 0xFF`、`high == type & ~0xFF`；`high == 0 || low != 0` 时比低字节，
      否则 `(id & high) == high`。`Mob = 0x100` 是「纯类别」→ 走位掩码分支，
      `Player(0x13F)` / `Zombie` 都命中，`PrimedTnt(0x41)` / `Minecart(0x80000)` 都不命中。）
      注意 `Actor::isType()`（`0x9BA9C0`）是**等值比较** `getEntityTypeId() == type`，不是位运算。
- [x] `fetchShapesHook` 保持原样转调（API 层无法重写，见 TODO-5）

## 待办

- [ ] **TODO-1 玩家逐轴**：`PlayerMoveSystemsImpl::doPlayerPostMoveSystem` 会调用
      `PlayerEventCoordinator::sendPlayerMove()`，分段执行会发出 N 次 `PlayerMoveEvent`。
      需要 hook 该函数 + thread_local 计数，只放行一次。
- [ ] **TODO-2 生物逐轴**：`Actor::checkFallDamage` 的非生物分支
      `if (ySpeed >= 0) mFallDistance = 0; else mFallDistance -= ySpeed;`
      在「该段 ySpeed == 0」时会把累计摔落距离清零 -> 循环前后对
      `FallDistanceComponent::mFallDistance` 做快照/恢复。
- [ ] **TODO-3 AutoStep**：`AutoStepRequestFlag` 每段都被 `RemoveFromAllEntitiesSystem` 清掉，
      `AutoStepFilterSystem` 又会重新置位 -> 可能重复上台阶，需要实测/抑制。
- [ ] **TODO-4 26.x 侧复核**：用可分析的 26.x `.i64` 重新枚举
      `VanillaSystemsRegistration::registerActorMoveSystems`（1.21.3 名单只能作参考），
      并确认 `MoveSpeedCapSystem` 的实际作用。
- [ ] **TODO-5 可选的「根治」方案**：用 `ll::memory::modify` 直接 NOP 掉 `fetchCollisionShapes`
      开头的 clamp（需要 26.x 中该处的 pattern/偏移）；成功后分段逻辑可以简化，甚至只保留拆轴。
- [ ] **TODO-6 影子模式对拍**：原版 vs 本方案，比对 `AABBShapeComponent` / `StateVectorComponent` /
      `MoveRequestComponent::mSpeed` 的终态是否一致（重点：贴墙、上台阶、跨 chunk 边界）。

## 已知风险

- 拆轴后每次 `computeMoveWithDepenetration` 拿到的碰撞形状集合/顺序与原版不同，
  slide 是逐形状累积的，因此**位置精确一致无法保证**（>16 区间本来也没有原版语义可对齐）。
- `MoveRequestComponent` 在 ActorMove 类别的最后会被 `RemoveFromAllEntitiesSystem` 移除，
  所以 `origin()` 返回后不能依赖该组件读取「本次实际位移」。