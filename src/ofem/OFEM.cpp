#include "ofem/OFEM.h"
#include "ofem/base/Hooks.h"

#include "ll/api/mod/RegisterHelper.h"

namespace ofem {

OFEM& OFEM::getInstance() {
    static OFEM instance;
    return instance;
}

bool OFEM::load() {
    ofem::base::HooksManager::getInstance().enable();
    return true;
}

bool OFEM::enable() {
    // 热重载
    ofem::base::HooksManager::getInstance().enable();
    return true;
}

bool OFEM::disable() {
    ofem::base::HooksManager::getInstance().disable();
    return true;
}

bool OFEM::unload() {
    // unload 前已经 disable, 无需重复 disable
    return true;
}

} // namespace ofem

LL_REGISTER_MOD(ofem::OFEM, ofem::OFEM::getInstance());
