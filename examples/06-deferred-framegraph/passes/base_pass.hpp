#pragma once

#include "vgfw.hpp"

#include <fg/Fwd.hpp>

class BasePass
{
public:
    explicit BasePass(vgfw::renderer::RenderContext& rc) : m_RenderContext(rc) {}
    ~BasePass() = default;

protected:
    vgfw::renderer::RenderContext&   m_RenderContext;
};