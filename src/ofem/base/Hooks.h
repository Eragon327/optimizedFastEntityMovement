#pragma once

namespace ofem::base {

class HooksManager {
private:
    bool enabled    = false;
    HooksManager()  = default;
    ~HooksManager() = default;

public:
    static HooksManager& getInstance();
    void                 enable();
    void                 disable();
};

} // namespace ofem::base