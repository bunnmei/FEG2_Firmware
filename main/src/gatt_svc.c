#include "gatt_svc.h"
#include "common.h"
#include "temp.h"
#include "tm1640.h"
#include "persistance.h"

static int temp_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg);

static const ble_uuid128_t temp_svc_uuid =
    BLE_UUID128_INIT(0xc0,0x43,0x12,0x76,0xc4,0x92,0x4d,0x80,0x35,0xb0,0x8c,0x71,0x4d,0x86,0x1b,0x81);

static float temp_chr_val[1] = {0};
static uint16_t temp_chr_val_handle;
static const ble_uuid128_t temp_chr_uuid = BLE_UUID128_INIT(0x84,0x2b,0xf3,0xd5,0x7f,0x88,0xc2,0xed,0xab,0xb5,0xca,0x0e,0x01,0xa0,0x01,0x06);

static float temp_s_chr_val[1] = {0};
static uint16_t temp_s_chr_val_handle;
static const ble_uuid128_t temp_s_chr_uuid = BLE_UUID128_INIT(0xe5,0xfb,0x02,0x7b,0xb8,0x7d,0x28,0x83,0x12,0xca,0xc6,0x17,0xe5,0x21,0x03,0x62);

static uint16_t temp_chr_conn_handle = 0;
static bool temp_chr_conn_handle_inited = false;
static bool temp_ind_status = false;

static uint16_t temp_s_chr_conn_handle = 0;
static bool temp_s_chr_conn_handle_inited = false;
static bool temp_s_ind_status = false;

// temp_calic
static int8_t temp_f_carib_val[1] = {0};
static uint16_t temp_f_carib_val_handle;
static const ble_uuid128_t temp_f_carib_chr_uuid = BLE_UUID128_INIT(0x41,0xeb,0x08,0x23,0x63,0xc5,0x88,0x8c,0x8f,0x26,0xde,0xee,0xa6,0x10,0x40,0x32);
static int8_t temp_f_carib_write_val[1] = {0};

static int8_t temp_s_carib_val[1] = {0};
static uint16_t temp_s_carib_val_handle;
static const ble_uuid128_t temp_s_carib_chr_uuid = BLE_UUID128_INIT(0x62,0xe0,0x12,0xa1,0xab,0xaf,0x74,0x11,0x40,0x35,0xe2,0x46,0xfa,0x5b,0x52,0x04);
static int8_t temp_s_carib_write_val[1] = {0};

// brightness service UUID
static uint8_t tm1640_brightness_val[1] = {0};
static uint16_t tm1640_brightness_val_handle;
static const ble_uuid128_t tm1640_brightness_chr_uuid = BLE_UUID128_INIT(0xff, 0x5f, 0x27, 0xf5, 0x37, 0x3b, 0xa3, 0xea, 0x75, 0x0c, 0x42, 0x3d, 0x93, 0x9d, 0x95, 0x8a);
static uint8_t tm1640_brightness_write_val[1] = {0};

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* temp service */
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &temp_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
            {/* temp one characteristic */
            .uuid = &temp_chr_uuid.u,
            .access_cb = temp_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            .val_handle = &temp_chr_val_handle},
            {/* temp two characteristic */
            .uuid = &temp_s_chr_uuid.u,
            .access_cb = temp_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            .val_handle = &temp_s_chr_val_handle},
            {/* temp one calibration characteristic */
            .uuid = &temp_f_carib_chr_uuid.u,
            .access_cb = temp_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            .val_handle = &temp_f_carib_val_handle},
            {/* temp two calibration characteristic */
            .uuid = &temp_s_carib_chr_uuid.u,
            .access_cb = temp_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            .val_handle = &temp_s_carib_val_handle},
            {/* tm1640 brightness characteristic */
            .uuid = &tm1640_brightness_chr_uuid.u,
            .access_cb = temp_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            .val_handle = &tm1640_brightness_val_handle},
            {
                0, /* No more characteristics in this service. */
            }
        }
    },

    {
        0, /* No more services. */
    },
};

/* Private functions */
static int temp_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc;

    /* Handle access events */
    /* Note:  characteristic is read only */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == temp_chr_val_handle) {
            /* Update access buffer value */
            temp_chr_val[0] = get_temp(TEMP_ONE);
            rc = os_mbuf_append(ctxt->om, &temp_chr_val,
                                sizeof(temp_chr_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        else if (attr_handle == temp_s_chr_val_handle) {
            /* Update access buffer value */
            temp_s_chr_val[0] = get_temp(TEMP_TWO);
            rc = os_mbuf_append(ctxt->om, &temp_s_chr_val,
                                sizeof(temp_s_chr_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }  else if (attr_handle == temp_f_carib_val_handle) {
            /* Update access buffer value */
            Persistance* persistance = get_persistance();
            temp_f_carib_val[0] = persistance->temp_f_caribration;
            printf("temp_f_carib_val read: %d\n", temp_f_carib_val[0]);
            rc = os_mbuf_append(ctxt->om, &temp_f_carib_val,
                                sizeof(temp_f_carib_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (attr_handle == temp_s_carib_val_handle) {
            /* Update access buffer value */
            Persistance* persistance = get_persistance();
            temp_s_carib_val[0] = persistance->temp_s_caribration;

            rc = os_mbuf_append(ctxt->om, &temp_s_carib_val,
                                sizeof(temp_s_carib_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } 
        else if (attr_handle == tm1640_brightness_val_handle) {

            Persistance* persistance = get_persistance();
            tm1640_brightness_val[0] = persistance->brightness_lvl;
            rc = os_mbuf_append(ctxt->om, &tm1640_brightness_val,
                                sizeof(tm1640_brightness_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }
        /* Verify attribute handle */
        if (attr_handle == temp_f_carib_val_handle) {
    
            int rc = ble_hs_mbuf_to_flat(ctxt->om, temp_f_carib_write_val, sizeof(temp_f_carib_write_val), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            printf("temp_f_carib_val catch: %d\n", temp_f_carib_write_val[0]);
            persistance_save(temp_f_carib_write_val[0], PERSISTANCE_TEMP_F_CARIBRATION);
            return rc;
            
        } else if (attr_handle == temp_s_carib_val_handle) {
  
            int rc = ble_hs_mbuf_to_flat(ctxt->om, temp_s_carib_write_val, sizeof(temp_s_carib_write_val), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            printf("temp_s_carib_val catch: %d\n", temp_s_carib_write_val[0]);
            persistance_save(temp_s_carib_write_val[0], PERSISTANCE_TEMP_S_CARIBRATION);
            return rc;

        } else if (attr_handle == tm1640_brightness_val_handle) {
            /* Update brightness value */
            int rc = ble_hs_mbuf_to_flat(ctxt->om, tm1640_brightness_write_val, sizeof(tm1640_brightness_write_val), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            printf("tm1640_brightness_val catch: %d\n", tm1640_brightness_write_val[0]);
            persistance_save(tm1640_brightness_write_val[0], PERSISTANCE_BRIGHTNESS_LVL);
            
            return rc;
        }
        goto error;
    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to heart rate characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

/* Public functions */
void send_temp_indication(void) {
    // printf("Sending temperature indication...\n");
    if (temp_ind_status && temp_chr_conn_handle_inited) {
        ble_gatts_indicate(temp_chr_conn_handle,
                           temp_chr_val_handle);
        ESP_LOGI(TAG, "temp indication sent!");
    }
    if (temp_s_ind_status && temp_s_chr_conn_handle_inited) {
        ble_gatts_indicate(temp_s_chr_conn_handle,
                           temp_s_chr_val_handle);
        ESP_LOGI(TAG, "temp_s indication sent!");
    }
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

/*
 *  GATT server subscribe event callback
 *      1. Update heart rate subscription status
 */

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    if (event->subscribe.attr_handle == temp_chr_val_handle) {
        /* Update temp subscription status */
        // printf("temp_chr_val_handle\n");
        temp_chr_conn_handle = event->subscribe.conn_handle;
        temp_chr_conn_handle_inited = true;
        temp_ind_status = event->subscribe.cur_notify;

        printf("temp_NOTIFI_CALLED_status: %d\n", temp_ind_status);
        // temp_ind_status = event->subscribe.cur_indicate;
    }

    if (event->subscribe.attr_handle == temp_s_chr_val_handle) {
        /* Update temp subscription status */
        // printf("temp_s_chr_val_handle\n");
        temp_s_chr_conn_handle = event->subscribe.conn_handle;
        temp_s_chr_conn_handle_inited = true;
        temp_s_ind_status = event->subscribe.cur_notify;
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
