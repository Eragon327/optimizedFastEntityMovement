#include "ofem/base/Hooks.h"

#include "ll/api/memory/Hook.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorClassTree.h"
#include "mc/world/actor/ActorType.h"


// 26.x 的 MoveCollisionSystem::fetchCollisionShapes 会在函数开头把 |mSpeed| 压到 16 格/gt。
// 只要"每一段"的位移都不超过 16，就永远不会触发那个上限，也就不必去改/重写它。
// （想直接摘掉那个上限的做法见 TODO.md 的 TODO-5。）
constexpr float kMaxStep = 16.0f;

namespace {

enum class kAxis { X, Y, Z };

[[nodiscard]] float lengthSqr(::Vec3 const& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }

[[nodiscard]] float axisComponent(::Vec3 const& v, kAxis axis) {
    return axis == kAxis::X ? v.x : axis == kAxis::Y ? v.y : v.z;
}

[[nodiscard]] ::Vec3 axisVector(float value, kAxis axis) {
    return {axis == kAxis::X ? value : 0.0f, axis == kAxis::Y ? value : 0.0f, axis == kAxis::Z ? value : 0.0f};
}

[[nodiscard]] bool isMobActor(::Actor const& actor) { return ::ActorClassTree::isInstanceOf(actor, ::ActorType::Mob); }

[[nodiscard]] bool shouldSplitMove(::Actor const& actor, ::Vec3 const& delta) {
    if (lengthSqr(delta) <= kMaxStep * kMaxStep) return false;
    return !isMobActor(actor);
}


} // namespace

// 高速拆轴：>16 格/gt 时按 Y->X->Z 拆轴、每轴再按 <=16 分段；每段都是一次完整的原版管线。
// Lowest 保证最后计算，不影响其他探针
LL_TYPE_INSTANCE_HOOK(ActorMoveHook, HookPriority::Lowest, Actor, &Actor::move, void, ::Vec3 const& posDelta) {
    if (!shouldSplitMove(*this, posDelta)) return origin(posDelta); // 原版，逐位一致

    for (kAxis axis : {kAxis::Y, kAxis::X, kAxis::Z}) {
        for (float remain = axisComponent(posDelta, axis); remain != 0.0f;) {
            float step  = std::clamp(remain, -kMaxStep, kMaxStep);
            remain     -= step;

            auto const before = this->getPosition();
            origin(axisVector(step, axis));
            auto const after = this->getPosition();

            // 这一轴被方块挡住：残量按原版语义丢弃，不再掰同一轴
            auto const dx = after.x - before.x;
            auto const dy = after.y - before.y;
            auto const dz = after.z - before.z;
            if (dx * dx + dy * dy + dz * dz <= 0.0f) break;
        }
    }
}

namespace ofem::base {

HooksManager& HooksManager::getInstance() {
    static HooksManager instance;
    return instance;
}

void HooksManager::enable() {
    if (enabled) return;
    enabled = true;
    ActorMoveHook::hook();
}

void HooksManager::disable() {
    if (!enabled) return;
    enabled = false;
    ActorMoveHook::unhook();
}

} // namespace ofem::base