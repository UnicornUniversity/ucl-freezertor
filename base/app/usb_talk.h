#ifndef _USB_TALK_H
#define _USB_TALK_H

#include <bc_common.h>
#include <jsmn.h>

#define USB_TALK_UINT_VALUE_NULL -1
#define USB_TALK_UINT_VALUE_INVALID -2

typedef struct
{
    const char *buffer;
    int token_count;
    jsmntok_t *tokens;

} usb_talk_payload_t;

typedef void (*usb_talk_sub_callback_t)(usb_talk_payload_t *payload);

void usb_talk_init(void);
void usb_talk_sub(const char *topic, void (*callback)(usb_talk_payload_t *));
void usb_talk_send_string(const char *buffer);
void usb_talk_publish_push_button(const char *prefix, uint16_t *event_count);
void usb_talk_publish_thermometer(const char *prefix, uint8_t *i2c, float *temperature);
void usb_talk_publish_humidity_sensor(const char *prefix, uint8_t *i2c, float *relative_humidity);
void usb_talk_publish_lux_meter(const char *prefix, uint8_t *i2c, float *illuminance);
void usb_talk_publish_barometer(const char *prefix, uint8_t *i2c, float *pascal, float *altitude);
void usb_talk_publish_light(const char *prefix, bool *state);
void usb_talk_publish_relay(const char *prefix, bool *state);
void usb_talk_publish_led_strip_config(const char *prefix, const char *sufix, const char *mode, int *count);

bool usb_talk_is_string_token_equal(const char *buffer, jsmntok_t *token, const char *string);
bool usb_talk_payload_get_bool(usb_talk_payload_t *payload, const char *key, bool *value);
bool usb_talk_payload_get_data(usb_talk_payload_t *payload, const char *key, uint8_t *buffer, size_t *length);
bool usb_talk_payload_get_enum(usb_talk_payload_t *payload, const char *key, int *value, ...);
bool usb_talk_payload_get_uint(usb_talk_payload_t *payload, const char *key, int *value);

#endif /* _USB_TALK_H */