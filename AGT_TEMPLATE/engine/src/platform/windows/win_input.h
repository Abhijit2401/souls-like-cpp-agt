#pragma once

#include "engine/core/input.h"

namespace engine
{
    class win_input : public input
    {
    public:
        bool key_pressed_impl(int32_t key_code) const override;
        bool mouse_button_pressed_impl(int32_t button) const override;

    protected:
        std::pair<float, float> mouse_position_impl() const override;
        float mouse_x_impl() const override;
        float mouse_y_impl() const override;

    private:
        inline static std::pair<float, float> s_last_position;
        // Tracks whether the cursor was hidden (mouse-look mode) on the previous
        // call, so the frame a menu closes can resync instead of computing a
        // delta against a stale position (see mouse_position_impl).
        inline static bool s_was_cursor_hidden = false;
    };
}
