#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

/* ================= CONFIGURATION ================= */

#define MAX_DEVICES     4
#define MAX_DATAPOINTS  8
#define MAX_STR_LEN     32
#define MAX_DATE_LEN    12
#define MAX_TIME_LEN    20

/* ================= DATA STRUCTURES ================= */

typedef struct {
    char timestamp[MAX_TIME_LEN];       /* "YYYY-MM-DD HH:MM" */
    char meter_datetime[MAX_TIME_LEN];  /* "YYYY-MM-DD HH:MM" */
    double total_value;
    int status;                         /* 0 = OK, 1 = ERROR */
} datapoint_t;

typedef struct {
    char media[MAX_STR_LEN];
    char meter[MAX_STR_LEN];
    char device_id[MAX_STR_LEN];
    char unit[MAX_STR_LEN];
    datapoint_t data[MAX_DATAPOINTS];
    int datapoint_count;
} device_reading_t;

typedef struct {
    int device_count;
    device_reading_t readings[MAX_DEVICES];
} values_t;

typedef struct {
    char gateway_id[MAX_STR_LEN];
    char date[MAX_DATE_LEN];
    char device_type[MAX_STR_LEN];
    int interval_minutes;
    int total_readings;
    values_t values;
} gateway_data_t;

typedef enum {
    JSON_OK = 0,
    JSON_ERR_NULL_PTR,
    JSON_ERR_BUFFER_TOO_SMALL,
    JSON_ERR_INVALID_INPUT
} json_status_t;

/* ================= HELPER FUNCTIONS ================= */

static const char *status_to_string(int status) {
    return (status == 0) ? "OK" : "ERROR";
}

/*
 * Safe append function. 
 * Returns 0 on success, -1 on error/overflow.
 */
static int append(char *buf, size_t buf_size, size_t *offset, const char *fmt, ...) {
    va_list args;
    int written;
    size_t remaining;

    if (*offset >= buf_size) {
        return -1; /* Buffer full */
    }

    remaining = buf_size - *offset;

    va_start(args, fmt);
    written = vsnprintf(buf + *offset, remaining, fmt, args);
    va_end(args);

    /* Check for encoding error or truncation */
    if (written < 0 || (size_t)written >= remaining) {
        return -1;
    }

    *offset += (size_t)written;
    return 0;
}

/* ================= SERIALIZER LOGIC ================= */

json_status_t serialize_to_json(const gateway_data_t *input, char *out_buffer, size_t buffer_size) {
    size_t offset = 0;
    int i, j;
    const device_reading_t *dev;
    const datapoint_t *dp;

    if (!input || !out_buffer) {
        return JSON_ERR_NULL_PTR;
    }

    if (input->values.device_count > MAX_DEVICES) {
        return JSON_ERR_INVALID_INPUT;
    }

    /* 1. Start JSON Array */
    if (append(out_buffer, buffer_size, &offset, "[{") < 0) {
        return JSON_ERR_BUFFER_TOO_SMALL;
    }

    /* 2. Serialize Gateway Metadata */
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
        input->total_readings) < 0) {
        return JSON_ERR_BUFFER_TOO_SMALL;
    }

    /* 3. Start Values Object */
    if (append(out_buffer, buffer_size, &offset,
        "\"values\":{"
        "\"device_count\":%d,"
        "\"readings\":[",
        input->values.device_count) < 0) {
        return JSON_ERR_BUFFER_TOO_SMALL;
    }

    /* 4. Loop over devices */
    for (i = 0; i < input->values.device_count; i++) {
        dev = &input->values.readings[i];

        if (dev->datapoint_count > MAX_DATAPOINTS) {
            return JSON_ERR_INVALID_INPUT;
        }

        /* Device Metadata */
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
            dev->unit) < 0) {
            return JSON_ERR_BUFFER_TOO_SMALL;
        }

        /* 5. Loop over datapoints */
        for (j = 0; j < dev->datapoint_count; j++) {
            dp = &dev->data[j];

            /* CRITICAL: Keys "meter datetime" and "total m3" must contain spaces. */
            if (append(out_buffer, buffer_size, &offset,
                "{"
                "\"timestamp\":\"%s\","
                "\"meter datetime\":\"%s\","  
                "\"total m3\":%.3f,"
                "\"status\":\"%s\""
                "}%s",
                dp->timestamp,
                dp->meter_datetime,
                dp->total_value,
                status_to_string(dp->status),
                (j < dev->datapoint_count - 1) ? "," : "") < 0) {
                return JSON_ERR_BUFFER_TOO_SMALL;
            }
        }

        /* Close Device Object */
        if (append(out_buffer, buffer_size, &offset,
            "]}%s",
            (i < input->values.device_count - 1) ? "," : "") < 0) {
            return JSON_ERR_BUFFER_TOO_SMALL;
        }
    }

    /* 6. Close Root Object */
    if (append(out_buffer, buffer_size, &offset, "]}}]") < 0) {
        return JSON_ERR_BUFFER_TOO_SMALL;
    }

    return JSON_OK;
}

/* ================= DEMO APPLICATION ================= */

int main(void) {
    gateway_data_t gateway;
    device_reading_t *dev;
    datapoint_t *dp;
    char json_buffer[2048];
    json_status_t result;

    /* Initialize memory to zero */
    memset(&gateway, 0, sizeof(gateway));

    /* --- Populate Sample Data --- */
    
    /* Gateway Info */
    strncpy(gateway.gateway_id, "gateway_1234", MAX_STR_LEN);
    strncpy(gateway.date, "1970-01-01", MAX_DATE_LEN);
    strncpy(gateway.device_type, "stromleser", MAX_STR_LEN);
    gateway.interval_minutes = 15;
    gateway.total_readings = 1;

    /* Device Info */
    gateway.values.device_count = 1;
    dev = &gateway.values.readings[0];
    strncpy(dev->media, "water", MAX_STR_LEN);
    strncpy(dev->meter, "waterstarm", MAX_STR_LEN);
    strncpy(dev->device_id, "stromleser_50898527", MAX_STR_LEN);
    strncpy(dev->unit, "m3", MAX_STR_LEN);
    
    /* Datapoint Info */
    dev->datapoint_count = 1;
    dp = &dev->data[0];
    strncpy(dp->timestamp, "1970-01-01 00:00", MAX_TIME_LEN);
    strncpy(dp->meter_datetime, "1970-01-01 00:00", MAX_TIME_LEN);
    dp->total_value = 107.752;
    dp->status = 0; 

    /* --- Execute Serialization --- */
    result = serialize_to_json(&gateway, json_buffer, sizeof(json_buffer));

    /* --- Print Result --- */
    if (result == JSON_OK) {
        printf("Serialized JSON:\n%s\n", json_buffer);
    } else {
        printf("Error: Serialization failed with code %d\n", result);
    }

    return 0;
}
