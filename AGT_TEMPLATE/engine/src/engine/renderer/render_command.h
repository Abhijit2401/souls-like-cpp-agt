#pragma once
#include "renderer_api.h"

namespace engine
{
    /// \brief
    class render_command
    {
    public:
        static void init()
        {
            renderer_api::init();
        }

        static void resize_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            renderer_api::resize_viewport(x,y,width, height);
        }

        static void clear_color(const glm::vec4 &color)
        {
            renderer_api::clear_color(color);
        }

        static void clear()
        {
            renderer_api::clear();
        }

        static void clear_depth()
        {
            renderer_api::clear_depth();
        }

        static void enable_depth_test()
        {
            renderer_api::enable_depth_test();
        }

        static void disable_depth_test()
        {
            renderer_api::disable_depth_test();
        }

        static void enable_culling()
        {
            renderer_api::enable_culling();
        }

        static void disable_culling()
        {
            renderer_api::disable_culling();
        }

        static void bind_texture(uint32_t texture_id, uint32_t slot)
        {
            renderer_api::bind_texture(texture_id, slot);
        }

        static void toggle_wireframe()
        {
            static bool is_wireframe = true;
            if(is_wireframe)
                renderer_api::enable_wireframe();
            else
                renderer_api::disable_wireframe();

            is_wireframe = !is_wireframe;
        }

        static void primitive_type(const e_primitive_type& type)
        {
            renderer_api::primitive_type(type);
        }

        static void submit(const ref<vertex_array>& vertex_array)
        {
            renderer_api::draw_indexed(vertex_array);
        }

    };
}
