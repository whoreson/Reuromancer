#include "globals.h"
#include "scene_control.h"
#include <stdio.h>
#include <string.h>

static void init(void) { }
static void deinit(void) { }
static void handle_input(sfEvent *event) { (void)event; }
static neuro_scene_id_t do_update(void) { return NSID_NOT_IMPLEMENTED; }

void setup_not_implemented_scene(void)
{
    g_scene.init = init;
    g_scene.deinit = deinit;
    g_scene.handle_input = handle_input;
    g_scene.update = do_update;
    g_scene.id = NSID_NOT_IMPLEMENTED;
}

void not_implemented_menu_handle_button_press(int *state, int button)
{
    (void)state; (void)button;
}
