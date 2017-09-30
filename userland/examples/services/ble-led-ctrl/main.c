#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ble_advdata.h>
#include <nordic_common.h>
#include <nrf_error.h>

#include <eddystone.h>
#include <simple_adv.h>
#include <simple_ble.h>

#include <ipc.h>
#include <nrf.h>
#include <nrf51_serialization.h>
#include <tock.h>

#include "led_ctrl_service.h"

/*******************************************************************************
 * BLE
 ******************************************************************************/

uint16_t conn_handle = BLE_CONN_HANDLE_INVALID;

volatile bool armed = false;

// Intervals for advertising and connections
simple_ble_config_t ble_config = {
  .platform_id       = 0x00,                // used as 4th octect in device BLE address
  .device_id         = DEVICE_ID_DEFAULT,
  .adv_name          = "TOCK-BTN",
  .adv_interval      = MSEC_TO_UNITS(500, UNIT_0_625_MS),
  .min_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),
  .max_conn_interval = MSEC_TO_UNITS(1250, UNIT_1_25_MS)
};

__attribute__ ((const))
void ble_address_set (void) {
  // nop
}

void ble_evt_user_handler (ble_evt_t* p_ble_evt) {
  ble_gap_conn_params_t conn_params;
  memset(&conn_params, 0, sizeof(conn_params));
  conn_params.min_conn_interval = ble_config.min_conn_interval;
  conn_params.max_conn_interval = ble_config.max_conn_interval;
  conn_params.slave_latency     = SLAVE_LATENCY;
  conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

  switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONN_PARAM_UPDATE:
      // just update them right now
      sd_ble_gap_conn_param_update(0, &conn_params);
      break;

    case BLE_GATTS_EVT_WRITE:
    {
      ble_gatts_evt_write_t* context = &p_ble_evt->evt.gatts_evt.params.write;
      btn_ctrl_handle_write(context);
      break;
    }
  }
}

void ble_evt_connected(ble_evt_t* p_ble_evt) {
  UNUSED_PARAMETER(p_ble_evt);
  printf("Connected to central\n");

  ble_common_evt_t *common = (ble_common_evt_t*)&p_ble_evt->evt;
  conn_handle = common->conn_handle;
}

void ble_evt_disconnected(ble_evt_t* p_ble_evt) {
  UNUSED_PARAMETER(p_ble_evt);
  printf("Disconnected from central\n");
  conn_handle = BLE_CONN_HANDLE_INVALID;
}

void ble_error (uint32_t error_code) {
  printf("BLE ERROR: Code = %d\n", (int)error_code);
}

// Will be called by the Simple BLE library.
void services_init (void) {
  btn_ctrl_service_init();
}

/*******************************************************************************
 * IPC
 ******************************************************************************/


static void ipc_callback(int pid, int len, int buf, __attribute__ ((unused)) void* ud) {
    printf("%d %d 0x%08x\n", pid, len, buf);

    if (!btn_ctrl_tx_arm(pid, buf)) {
        printf("Error! Already armed!\n");
    } else {
        printf("Armed!\n");
    }
//   if (len < (int) sizeof(sensor_update_t)) {
//     printf("Error! IPC message too short.\n");
//     ipc_notify_client(pid);
//     return;
//   }

//   sensor_update_t *update = (sensor_update_t*) buf;

//   if (conn_handle != BLE_CONN_HANDLE_INVALID) {
//     switch (update->type) {
//       case SENSOR_TEMPERATURE:
//         // env_sense_update_temperature(conn_handle, update->value);
//         break;
//       case SENSOR_IRRADIANCE:
//         // env_sense_update_irradiance(conn_handle,  update->value);
//         break;
//       case SENSOR_HUMIDITY:
//         // env_sense_update_humidity(conn_handle,  update->value);
//         break;
//     }
//   }
//   ipc_notify_client(pid);
}

/*******************************************************************************
 * MAIN
 ******************************************************************************/

int main (void) {
 printf("[BLE] Buttons IPC Service\n");
//
//  // Listen for IPC requests to configure the sensor values.
 ipc_register_svc(ipc_callback, NULL);
//
 // Setup BLE
 conn_handle = simple_ble_init(&ble_config)->conn_handle;
//
 // Advertise the BLE environmental sensing service.
 ble_uuid_t adv_uuid = {
   .uuid = 0x0101, // TODO advertise custom
   .type = BLE_UUID_TYPE_BLE
 };
 simple_adv_service(&adv_uuid);
}
