#include <application.h>
#include <bcl.h>

#define PREFIX_TALK "fridge-monitor"
#define MAX_COUNTER 300

struct { float temperature[300]; float illuminance[300]; uint counter; } measurement;

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
        event_counter++;
        bc_radio_pub_push_button(&event_counter);
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        bc_radio_enroll_to_gateway();
        bc_led_pulse(&led, 1000);
    }
}

void application_init(void) {
    measurement.counter = 0;

    usb_talk_init();

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize radio
    bc_radio_init();

    bc_module_climate_init();
    bc_module_climate_set_update_interval_thermometer(10000);
    bc_module_climate_set_update_interval_lux_meter(10000);
    bc_module_climate_set_event_handler(climate_event_event_handler, NULL);
}

void climate_event_event_handler(bc_module_climate_event_t event, void *event_param)
{
    (void) event_param;
    float value;

    static uint8_t i2c_thermometer = 0x48;
    static uint8_t i2c_lux_meter = 0x44;

    switch (event)
    {
        case BC_MODULE_CLIMATE_EVENT_UPDATE_THERMOMETER:
            if (bc_module_climate_get_temperature_celsius(&value))
                {
                    //usb_talk_publish_thermometer(PREFIX_TALK, &i2c_thermometer, &value);
                    bc_radio_pub_thermometer(i2c_thermometer, &value);
                    //measurement.temperature[measurement.counter] = value;
                }
                break;

        case BC_MODULE_CLIMATE_EVENT_UPDATE_LUX_METER:
            if (bc_module_climate_get_luminosity_lux(&value))
            {
                //usb_talk_publish_lux_meter(PREFIX_TALK, &i2c_lux_meter, &value);
                bc_radio_pub_luminosity(i2c_lux_meter, &value);
                //measurement.illuminance[measurement.counter] = value;
            }
            break;

        case BC_MODULE_CLIMATE_EVENT_ERROR_HYGROMETER:
        case BC_MODULE_CLIMATE_EVENT_ERROR_BAROMETER:
        case BC_MODULE_CLIMATE_EVENT_ERROR_LUX_METER:
        case BC_MODULE_CLIMATE_EVENT_ERROR_THERMOMETER:
        case BC_MODULE_CLIMATE_EVENT_UPDATE_HYGROMETER:
        case BC_MODULE_CLIMATE_EVENT_UPDATE_BAROMETER:
        default:
            break;
    }

    measurement.counter++;

    if (measurement.counter == 300) { 
        measurement.counter = 0;
    }
    
}

