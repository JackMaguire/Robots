import arcade
 
width = 45
height = 30

box_size = 20
canvas_width = width * box_size
canvas_height = height * box_size

color="#CCCCCC"

def draw_background():
    """
    This function draws the background. Specifically, the sky and ground.
    """
    # Draw the sky in the top two-thirds
    arcade.draw_lrtb_rectangle_filled(0,
                                      canvas_width,
                                      canvas_height,
                                      0,
                                      arcade.color.WHITE)

    for i in range( 0, int(width / 2) + 1 ):
        for k in range( 0, height ):
            j = i * 2 +( k % 2 )
            x1 = j * box_size
            y1 = k * box_size
            x2 = x1 + box_size
            y2 = y1 + box_size
            arcade.draw_lrtb_rectangle_filled( x1, x2, y2, y1, arcade.color.GRAY );


def main():
    # Open the window
    arcade.open_window(canvas_width, canvas_height, "robots")

    # Start the render process. This must be done before any drawing commands.
    arcade.start_render()

    # Call our drawing functions.
    draw_background()
    #draw_pine_tree(50, 250)
    #draw_pine_tree(350, 320)
    #draw_bird(70, 500)
    #draw_bird(470, 550)

    # Finish the render.
    # Nothing will be drawn without this.
    # Must happen after all draw commands
    arcade.finish_render()

    # Keep the window up until someone closes it.
    arcade.run()

main()
