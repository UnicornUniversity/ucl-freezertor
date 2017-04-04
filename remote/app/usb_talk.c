#include <usb_talk.h>
#include <bc_scheduler.h>
#include <bc_usb_cdc.h>
#include <base64.h>
#include <application.h>

#define USB_TALK_TOKEN_ARRAY         0
#define USB_TALK_TOKEN_TOPIC         1
#define USB_TALK_TOKEN_PAYLOAD       2
#define USB_TALK_TOKEN_PAYLOAD_KEY   3
#define USB_TALK_TOKEN_PAYLOAD_VALUE 4

#define USB_TALK_SUBSCRIBES 10

static struct
{
    char tx_buffer[256];
    char rx_buffer[1024];
    size_t rx_length;
    bool rx_error;

    struct {
        const char *topic;
        usb_talk_sub_callback_t callback;

    } subscribes[USB_TALK_SUBSCRIBES];
    size_t subscribes_length;

} _usb_talk;

static void _usb_talk_task(void *param);
static void _usb_talk_process_character(char character);
static void _usb_talk_process_message(char *message, size_t length);

static int usb_talk_token_get_uint(const char *buffer, jsmntok_t *token);

void usb_talk_init(void)
{
    memset(&_usb_talk, 0, sizeof(_usb_talk));

    bc_usb_cdc_init();

    bc_scheduler_register(_usb_talk_task, NULL, 0);
}

void usb_talk_sub(const char *topic, usb_talk_sub_callback_t callback)
{
    if (_usb_talk.subscribes_length >= USB_TALK_SUBSCRIBES){
        return;
    }
    _usb_talk.subscribes[_usb_talk.subscribes_length].topic = topic;
    _usb_talk.subscribes[_usb_talk.subscribes_length].callback = callback;
    _usb_talk.subscribes_length++;
}

void usb_talk_send_string(const char *buffer)
{
    bc_usb_cdc_write(buffer, strlen(buffer));
}

void usb_talk_publish_push_button(const char *prefix, uint16_t *event_count)
{
    snprintf(_usb_talk.tx_buffer, sizeof(_usb_talk.tx_buffer),
                     "[\"%s/push-button/-\", {\"event-count\": %" PRIu16 "}]\n", prefix, *event_count);

    usb_talk_send_string((const char *) _usb_talk.tx_buffer);
}

void usb_talk_publish_thermometer(const char *prefix, uint8_t *i2c, float *temperature)
{

    snprintf(_usb_talk.tx_buffer, sizeof(_usb_talk.tx_buffer),
                "[\"%s/thermometer/i2c%d-%02x\", {\"temperature\": [%0.2f, \"\\u2103\"]}]\n",
                prefix, ((*i2c & 0x80) >> 7), (*i2c & ~0x80), *temperature);

    usb_talk_send_string((const char *) _usb_talk.tx_buffer);
}

void usb_talk_publish_humidity_sensor(const char *prefix, uint8_t *i2c, float *relative_humidity)
{
    snprintf(_usb_talk.tx_buffer, sizeof(_usb_talk.tx_buffer),
                    "[\"%s/humidity-sensor/i2c%d-%02x\", {\"relative-humidity\": [%0.1f, \"%%\"]}]\n",
                    prefix, ((*i2c & 0x80) >> 7), (*i2c & ~0x80), *relative_humidity);

    usb_talk_send_string((const char *) _usb_talk.tx_buffer);
}

void usb_talk_publish_lux_meter(const char *prefix, uint8_t *i2c, float *illuminance)
{
    snprintf(_usb_talk.tx_buffer, sizeof(_usb_talk.tx_buffer),
                    "[\"%s/lux-meter/i2c%d-%02x\", {\"illuminance\": [%0.1f, \"lux\"]}]\n",
                    prefix, ((*i2c & 0x80) >> 7), (*i2c & ~0x80), *illuminance);

    usb_talk_send_string((const char *) _usb_talk.tx_buffer);
}

void usb_talk_publish_barometer(const char *prefix, uint8_t *i2c, float *pressure, float *altitude)
{
    snprintf(_usb_talk.tx_buffer, sizeof(_usb_talk.tx_buffer),
                        "[\"%s/barometer/i2c%d-%02x\", {\"pressure\": [%0.2f, \"kPa\"], \"altitude\": [%0.2f, \"m\"]}]\n",
                        prefix, ((*i2c & 0x80) >> 7), (*i2c & ~0x80), *pressure, *altitude);

    usb_talk_send_string((const char *) _usb_talk.tx_buffer);
}

void usb_talk_publish_light(const char *prefix, bool *state)
{
    snprintf(_usb_talk.tx_buffer, sizeof(_usb_talk.tx_buffer), "[\"%s/light/-\", {\"state\": %s}]\n",
            prefix, *state ? "true" : "false");

    usb_talk_send_string((const char *) _usb_talk.tx_buffer);
}


void usb_talk_publish_relay(const char *prefix, bool *state)
{
    snprintf(_usb_talk.tx_buffer, sizeof(_usb_talk.tx_buffer), "[\"%s/relay/-\", {\"state\": %s}]\n",
            prefix, *state ? "true" : "false");

    usb_talk_send_string((const char *) _usb_talk.tx_buffer);
}


void usb_talk_publish_led_strip_config(const char *prefix, const char *sufix, const char *mode, int *count)
{
    snprintf(_usb_talk.tx_buffer, sizeof(_usb_talk.tx_buffer), "[\"%s/led-strip/-/config%s\", {\"mode\": \"%s\", \"count\": %d}]\n",
            prefix, sufix, mode, *count );

    usb_talk_send_string((const char *) _usb_talk.tx_buffer);
}

static void _usb_talk_task(void *param)
{
    (void) param;

    while (true)
    {
        static uint8_t buffer[16];

        size_t length = bc_usb_cdc_read(buffer, sizeof(buffer));

        if (length == 0)
        {
            break;
        }

        for (size_t i = 0; i < length; i++)
        {
            _usb_talk_process_character((char) buffer[i]);
        }
    }

    bc_scheduler_plan_current_now();
}

static void _usb_talk_process_character(char character)
{
    if (character == '\n')
    {
        if (!_usb_talk.rx_error && _usb_talk.rx_length > 0)
        {
            _usb_talk_process_message(_usb_talk.rx_buffer, _usb_talk.rx_length);
        }

        _usb_talk.rx_length = 0;
        _usb_talk.rx_error = false;

        return;
    }

    if (_usb_talk.rx_length == sizeof(_usb_talk.rx_buffer))
    {
        _usb_talk.rx_error = true;
    }
    else
    {
        if (!_usb_talk.rx_error)
        {
            _usb_talk.rx_buffer[_usb_talk.rx_length++] = character;
        }
    }
}

static void _usb_talk_process_message(char *message, size_t length)
{
    static jsmn_parser parser;
    static jsmntok_t tokens[16];

    jsmn_init(&parser);

    int token_count = jsmn_parse(&parser, (const char *) message, length, tokens, sizeof(tokens));

    if (token_count < 3)
    {
        return;
    }

    if (tokens[USB_TALK_TOKEN_ARRAY].type != JSMN_ARRAY || tokens[USB_TALK_TOKEN_ARRAY].size != 2)
    {
        return;
    }

    if (tokens[USB_TALK_TOKEN_TOPIC].type != JSMN_STRING || tokens[USB_TALK_TOKEN_TOPIC].size != 0)
    {
        return;
    }

    if (tokens[USB_TALK_TOKEN_PAYLOAD].type != JSMN_OBJECT)
    {
        return;
    }

    for (size_t i = 0; i < _usb_talk.subscribes_length; i++)
    {
        if (usb_talk_is_string_token_equal(message, &tokens[USB_TALK_TOKEN_TOPIC], _usb_talk.subscribes[i].topic))
        {
            usb_talk_payload_t payload = {
                    message,
                    token_count - USB_TALK_TOKEN_PAYLOAD_KEY,
                    tokens + USB_TALK_TOKEN_PAYLOAD_KEY
            };
            _usb_talk.subscribes[i].callback(&payload);
        }
    }
}

bool usb_talk_is_string_token_equal(const char *buffer, jsmntok_t *token, const char *string)
{
    size_t token_length;

    token_length = (size_t) (token->end - token->start);

    if (strlen(string) != token_length)
    {
        return false;
    }

    if (strncmp(string, &buffer[token->start], token_length) != 0)
    {
        return false;
    }

    return true;
}

bool usb_talk_payload_get_bool(usb_talk_payload_t *payload, const char *key, bool *value)
{
    for (int i = 0; i + 1 < payload->token_count; i+=2)
    {
        if (usb_talk_is_string_token_equal(payload->buffer, &payload->tokens[i], key))
        {
            if (usb_talk_is_string_token_equal(payload->buffer, &payload->tokens[i + 1], "true"))
            {
                *value = true;
                return true;
            }
            else if (usb_talk_is_string_token_equal(payload->buffer, &payload->tokens[i + 1], "false"))
            {
                *value = false;
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    return false;
}

bool usb_talk_payload_get_data(usb_talk_payload_t *payload, const char *key, uint8_t *buffer, size_t *length)
{
    for (int i = 0; i + 1 < payload->token_count; i+=2)
    {
        if (usb_talk_is_string_token_equal(payload->buffer, &payload->tokens[i], key))
        {
            if (payload->tokens[i + 1].type != JSMN_STRING)
            {
                return false;
            }

            uint32_t input_length = payload->tokens[i + 1].end - payload->tokens[i + 1].start;

            size_t data_length = base64_calculate_decode_length(&payload->buffer[payload->tokens[i + 1].start], input_length);

            if (data_length > *length)
            {
                return false;
            }

            return base64_decode(&payload->buffer[payload->tokens[i + 1].start], input_length, buffer, (uint32_t *)length);
        }
    }
    return false;
}

bool usb_talk_payload_get_enum(usb_talk_payload_t *payload, const char *key, int *value, ...)
{
    char temp[11];
    char *str;
    int j = 0;

    for (int i = 0; i + 1 < payload->token_count; i+=2)
    {
        if (usb_talk_is_string_token_equal(payload->buffer, &payload->tokens[i], key))
        {

            jsmntok_t *token = &payload->tokens[i + 1];
            size_t length = token->end - token->start;

            if (length > (sizeof(temp) - 1))
            {
                return false;
            }

            memset(temp, 0x00, sizeof(temp));

            strncpy(temp, payload->buffer + token->start, length);

            va_list vl;
            va_start(vl, value);
            str = va_arg(vl, char*);
            while (str != NULL)
            {
                if (strcmp(str, temp) == 0)
                {
                    *value = j;
                    return true;
                }
                str = va_arg(vl, char*);
                j++;
            }
            va_end(vl);

            return false;
        }
    }
    return false;
}

bool usb_talk_payload_get_uint(usb_talk_payload_t *payload, const char *key, int *value)
{
    for (int i = 0; i + 1 < payload->token_count; i+=2)
    {
        if (usb_talk_is_string_token_equal(payload->buffer, &payload->tokens[i], key))
        {
            *value = usb_talk_token_get_uint(payload->buffer, &payload->tokens[i + 1]);
            return *value == USB_TALK_UINT_VALUE_INVALID ? false : true;
        }
    }
    return false;
}

static int usb_talk_token_get_uint(const char *buffer, jsmntok_t *token)
{
    if (token->type != JSMN_PRIMITIVE)
    {
        return USB_TALK_UINT_VALUE_INVALID;
    }

    size_t length = (size_t) (token->end - token->start);

    char str[10 + 1];

    if (length > sizeof(str))
    {
        return USB_TALK_UINT_VALUE_INVALID;
    }

    memset(str, 0, sizeof(str));

    strncpy(str, buffer + token->start, length);

    if (strcmp(str, "null") == 0)
    {
        return USB_TALK_UINT_VALUE_NULL;
    }

    int ret;

    if (strchr(str, 'e'))
    {
        ret = (int) strtof(str, NULL);
    }
    else
    {
        ret = (int) strtol(str, NULL, 10);
    }

    if (ret < 0)
    {
        return USB_TALK_UINT_VALUE_INVALID;
    }

    return ret;
}