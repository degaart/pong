#pragma once

struct Keystate
{
};

class Engine
{
public:
    const KeyState& keystate() = 0;
};

struct Scene
{
    virtual ~Scene() = default;
    virtual void onInit() = 0;
    virtual bool onUpdate() = 0;
    virtual void onRender() = 0;
};


