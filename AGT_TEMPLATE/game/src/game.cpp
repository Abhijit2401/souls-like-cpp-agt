#include "example_layer.h"
#include "engine/core/entry_point.h"
#include "engine/events/key_event.h"


class sandbox_app : public engine::application
{
public:
    // Create and push the main game layer
    sandbox_app()
    {
        push_layer(new example_layer());
    }

    ~sandbox_app() = default;

    //Escape now opens the pause menu in example_layer instead of closing the game, quitting is done from the menus
};


engine::application* engine::create_application()
{
    return new sandbox_app();
}
