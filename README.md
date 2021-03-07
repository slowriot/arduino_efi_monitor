# Arduino EFI Monitor

A simple injector simulator to run on Arduino, and output to an RGB LED grid.

Intended for testing automotive ECU outputs for fuel injectors, primarily for DIY ECU projects such as RusEFI and Speeduino, but should work with any automotive ECU low-side outputs.

![](https://user-images.githubusercontent.com/649131/110259391-5b1bac80-7f9f-11eb-822f-5a86ea515201.gif)

Each column of LEDs corresponds to a cylinder of the engine.  The first LED of the column will flash white while the injector is commanded to open, and then change colour depending on firing order.  If this was the correct cylinder in the firing order, it turns green, otherwise it turns red, and highlights the correct cylinder (that should have fired) in yellow.  The rest of the LEDs in the column provide a timeline, scrolling one step along for each new pulse that's received.

Specify your number of cylinders (`num_cyls`) and expected `firing_order` at the top of the sketch, and connect ECU outputs to the pins specified in `cylinder_pins`, and ground to the ECU's output (power) ground.

The sketch is intended for Arduino Nano and clones but should work on anything with enough inputs.  The example is configured to use a Keyes WS2812 4x8 array, but any rectangular WS2812 array will work, just set `num_led_cols` and `num_led_rows` appropriately.  This should work fine with as few LEDs as your engine has cylinders - any additional column length just adds longer visual history for each cylinder.

Discussion thread: https://rusefi.com/forum/viewtopic.php?f=2&t=1959&p=40053#p40053

