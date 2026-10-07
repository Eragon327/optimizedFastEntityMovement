#include "ofem/base/Hooks.h"

#include "ll/api/memory/Hook.h"

#include "mc/entity/systems/move_collision_system/MoveCollisionSystem.h"


LL_STATIC_HOOK(
    fetchShapesHook,
    HookPriority::Normal,
    MoveCollisionSystem::fetchCollisionShapes,
    void,
    ::StrictEntityContext const&                       entity,
    ::AABBShapeComponent const&                        aabb,
    ::MaxAutoStepComponent const&                      autoStep,
    ::Optional<::CollidableMobNearFlagComponent const> collidableMobNear,
    ::MoveRequestComponent&                            request,
    ::Optional<::MinecartFlagComponent const>          isMinecart,
    ::ViewT<::StrictEntityContext, ::Include<::CollidableMobFlagComponent>, ::AABBShapeComponent const> const&
                                                                                                      collidableMobs,
    ::ViewT<::StrictEntityContext, ::AABBShapeComponent const, ::ActorDataFlagComponent const> const& stackableView,
    ::ViewT<::StrictEntityContext, ::Include<::FallingBlockFlagComponent>> const&                     fallingBlocks,
    ::IConstBlockSource const&                                                                        region,
    ::LocalSpatialEntityFetcher&                                                                      fetcher,
    ::GetCollisionShapeInterface const&                                                               collisionShape,
    ::std::vector<::BlockSourceVisitor::CollisionShape>& tempCollisionShapes,
    ::std::vector<::BlockSourceVisitor::CollisionShape>& scratchCollisionShapes,
    ::std::vector<::AABB>&                               tempShapes
) {
    origin(
        entity,
        aabb,
        autoStep,
        collidableMobNear,
        request,
        isMinecart,
        collidableMobs,
        stackableView,
        fallingBlocks,
        region,
        fetcher,
        collisionShape,
        tempCollisionShapes,
        scratchCollisionShapes,
        tempShapes
    );
}


namespace ofem::base {

HooksManager& HooksManager::getInstance() {
    static HooksManager instance;
    return instance;
}

void HooksManager::enable() {
    if (enabled) return;
    enabled = true;
    fetchShapesHook::hook();
}

void HooksManager::disable() {
    if (!enabled) return;
    enabled = false;
    fetchShapesHook::unhook();
}

} // namespace ofem::base