#include <application.h>

#define PREFIX_TALK_BASE "freezetor001-base"
#define PREFIX_TALK_REMOTE "freezetor001-remote"

// LED instance
bc_led_t led;

// Button instance
bc_button_t button;

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_pulse(&led, 100);
        static uint16_t event_counter = 0;
        usb_talk_publish_push_button(PREFIX_TALK_BASE, &event_counter);
        event_counter++;
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        bc_radio_enrollment_start();
        bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);
    }
}

void radio_event_handler(bc_radio_event_t event, void *event_param)
{
    (void) event_param;

    if (event == BC_RADIO_EVENT_PAIR_SUCCESS)
    {
        bc_radio_enrollment_stop();

        bc_led_pulse(&led, 1000);

        bc_led_set_mode(&led, BC_LED_MODE_OFF);
    }
}

void bc_radio_on_push_button(uint32_t *peer_device_address, uint16_t *event_count)
{
    (void) peer_device_address;

    bc_led_pulse(&led, 1000);
    usb_talk_publish_push_button(PREFIX_TALK_REMOTE, event_count);
}

void bc_radio_on_thermometer(uint32_t *peer_device_address, uint8_t *i2c, float *temperature)
{
    (void) peer_device_address;

    bc_led_pulse(&led, 10);

    usb_talk_publish_thermometer(PREFIX_TALK_REMOTE, i2c, temperature);
}

void bc_radio_on_lux_meter(uint32_t *peer_device_address, uint8_t *i2c, float *illuminance)
{
    (void) peer_device_address;

    bc_led_pulse(&led, 10);

    usb_talk_publish_lux_meter(PREFIX_TALK_REMOTE, i2c, illuminance);
}

void application_init(void) {
    usb_talk_init();

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize radio
    bc_radio_init();
    bc_radio_set_event_handler(radio_event_handler, NULL);
    bc_radio_listen();

    bc_module_climate_init();
}

void application_task()
{


}