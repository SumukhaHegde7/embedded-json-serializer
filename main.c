#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

/* ================= CONFIGURATION LIMITS ================= */

#define MAX_DEVICES     4
#define MAX_DATAPOINTS  8
#define MAX_STR_LEN     32

/* ================= DATA STRUCTURES ================= */

/* One measurement datapoint */
typedef struct {
    char timestamp[20];          // "YYYY-MM-DD HH:MM"
    char meter_datetime[20];     // "YYYY-MM-DD HH:MM"
    double total_value;          // numeric value (e.g. m3)
    int status;                  // 0 = OK, 1 = ERROR
} datapoint_t;

/* One device reading */
typedef struct {
    char media[MAX_STR_LEN];
    char meter[MAX_STR_LEN];
    char device_id[MAX_STR_LEN];
    char unit[MAX_STR_LEN];

    datapoint_t data[MAX_DATAPOINTS];
    int datapoint_count;
} device_reading_t;

/* Values container */
typedef struct {
    int device_count;
    device_reading_t readings[MAX_DEVICES];
} values_t;

/* Gateway root object */
typedef struct {
    char gateway_id[MAX_STR_LEN];
    char date[11];               // YYYY-MM-DD
    char device_type[MAX_STR_LEN];
    int interval_minutes;
    int total_readings;

    values_t values;
} gateway_data_t;

/* ================= ERROR CODES ================= */

typedef enum {
    JSON_OK = 0,
    JSON_ERR_NULL_PTR,
    JSON_ERR_BUFFER_TOO_SMALL,
    JSON_ERR_INVALID_INPUT
} json_status_t;

/* ================= INTERNAL HELPERS ================= */

/* Convert status code to JSON string */
static const char *status_to_string(int status) {
    return (status == 0) ? "OK" : "ERROR";
}

/* Safe append helper (prevents buffer overflow) */
static int append(char *buf, size_t buf_size, size_t *offset,
                  const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int written = vsnprintf(buf + *offset,
                            buf_size - *offset,
                            fmt, args);

    va_end(args);

    if (written < 0 || (size_t)written >= buf_size - *offset) {
        return -1;
    }

    *offset += written;
    return 0;
}

/* ================= JSON SERIALIZER ================= */

json_status_t serialize_to_json(
    const gateway_data_t *input,
    char *out_buffer,
    size_t buffer_size
) {
    if (!input || !out_buffer) {
        return JSON_ERR_NULL_PTR;
    }

    if (input->values.device_count > MAX_DEVICES) {
        return JSON_ERR_INVALID_INPUT;
    }

    size_t offset = 0;

    /* Start outer JSON array */
    if (append(out_buffer, buffer_size, &offset, "[{") < 0)
        return JSON_ERR_BUFFER_TOO_SMALL;

    /* Gateway metadata */
    if (append(out_buffer, buffer_size, &offset,
        "\"gatewayId\":\"%s\","
        "\"date\":\"%s\","
        "\"deviceType\":\"%s\","
        "\"interval_minutes\":%d,"
        "\"total_readings\":%d,",
        input->gateway_id,
        input->date,
        input->device_type,
        input->interval_minutes,
        input->total_readings) < 0)
        return JSON_ERR_BUFFER_TOO_SMALL;

    /* Values object */
    if (append(out_buffer, buffer_size, &offset,
        "\"values\":{"
        "\"device_count\":%d,"
        "\"readings\":[",
        input->values.device_count) < 0)
        return JSON_ERR_BUFFER_TOO_SMALL;

    /* Device loop */
    for (int i = 0; i < input->values.device_count; i++) {
        const device_reading_t *dev = &input->values.readings[i];

        if (dev->datapoint_count > MAX_DATAPOINTS) {
            return JSON_ERR_INVALID_INPUT;
        }

        if (append(out_buffer, buffer_size, &offset,
            "{"
            "\"media\":\"%s\","
            "\"meter\":\"%s\","
            "\"deviceId\":\"%s\","
            "\"unit\":\"%s\","
            "\"data\":[",
            dev->media,
            dev->meter,
            dev->device_id,
            dev->unit) < 0)
            return JSON_ERR_BUFFER_TOO_SMALL;

        /* Datapoint loop */
        for (int j = 0; j < dev->datapoint_count; j++) {
            const datapoint_t *dp = &dev->data[j];

            if (append(out_buffer, buffer_size, &offset,
                "{"
                "\"timestamp\":\"%s\","
                "\"meter_datetime\":\"%s\","
                "\"total_m3\":%.3f,"
                "\"status\":\"%s\""
                "}%s",
                dp->timestamp,
                dp->meter_datetime,
                dp->total_value,
                status_to_string(dp->status),
                (j < dev->datapoint_count - 1) ? "," : "") < 0)
                return JSON_ERR_BUFFER_TOO_SMALL;
        }

        if (append(out_buffer, buffer_size, &offset,
            "]}"
            "%s",
            (i < input->values.device_count - 1) ? "," : "") < 0)
            return JSON_ERR_BUFFER_TOO_SMALL;
    }

    /* Close all objects */
    if (append(out_buffer, buffer_size, &offset, "]}}]") < 0)
        return JSON_ERR_BUFFER_TOO_SMALL;

    return JSON_OK;
}

/* ================= DEMO APPLICATION ================= */

int main(void) {
    gateway_data_t gateway = {0};

    strcpy(gateway.gateway_id, "gateway_1234");
    strcpy(gateway.date, "1970-01-01");
    strcpy(gateway.device_type, "stromleser");
    gateway.interval_minutes = 15;
    gateway.total_readings = 1;

    gateway.values.device_count = 1;

    device_reading_t *dev = &gateway.values.readings[0];
    strcpy(dev->media, "water");
    strcpy(dev->meter, "waterstarm");
    strcpy(dev->device_id, "stromleser_50898527");
    strcpy(dev->unit, "m3");

    dev->datapoint_count = 1;

    datapoint_t *dp = &dev->data[0];
    strcpy(dp->timestamp, "1970-01-01 00:00");
    strcpy(dp->meter_datetime, "1970-01-01 00:00");
    dp->total_value = 107.752;
    dp->status = 0;

    char json_buffer[1024];

    json_status_t result =
        serialize_to_json(&gateway, json_buffer, sizeof(json_buffer));

    if (result == JSON_OK) {
        printf("%s\n", json_buffer);
    } else {
        printf("Serialization failed (error=%d)\n", result);
    }

    return 0;
}
