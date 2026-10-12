#pragma once

enum class ButtonAction {
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    SELECT
};

class IInputController {
public:
    virtual ~IInputController() = default;
    virtual void init() = 0;
    virtual ButtonAction getAction() = 0;
};
