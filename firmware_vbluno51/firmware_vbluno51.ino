/*
 * Project:     Dieu khien/Giam sat thiet bi bang Mobile qua BLE
 * Author:      VNG IoT Lab
 * Date:        8/2017
 * Hardware:    Bo mach VBLUno51 cua VNG
 *                Wiki: https://vngiotlab.github.io/vbluno/
 *                Sale: http://iotviet.com.vn/store/detail?id=2           
 * Software:    Arduino IDE
 * Description: 
 *     Tao 1 service BLE co 2 characteristic
 *          1. LED: Mobile dieu khien bat tat LED tren mach VBLUno51
 *          2. BUTTON: Mobile nhan gia tri tu VBLUno51 moi khi nhan Nut bam tren mach VBLUno51
 */
 
#include <BLE_API.h>

#define DEVICE_NAME       "LED_BUTTON_51"
#define TXRX_BUF_LEN      2

BLE                       ble;

static const uint8_t led_button_service_uuid[]    = {0x00, 0x00, 0xFF, 0xF8, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
static const uint8_t led_char_uuid[]              = {0x00, 0x00, 0xFF, 0xF9, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};  
static const uint8_t button_char_uuid[]           = {0x00, 0x00, 0xFF, 0xFA, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};  

static const uint8_t  led_button_uuid[]           = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF8, 0xFF, 0x00, 0x00};

uint8_t led_char_value[TXRX_BUF_LEN]              = {0};
uint8_t button_char_value[TXRX_BUF_LEN]           = {0};
                        
GattCharacteristic  led_characteristic(led_char_uuid, 
                                       led_char_value, 
                                       1, 
                                       TXRX_BUF_LEN, 
                                       GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE );

GattCharacteristic  button_characteristic(button_char_uuid, 
                                          button_char_value, 
                                          1, 
                                          TXRX_BUF_LEN, 
                                          GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);
GattCharacteristic *led_button_chars[] = {&led_characteristic, &button_characteristic};

GattService         led_button_service(led_button_service_uuid, 
                                       led_button_chars, 
                                       sizeof(led_button_chars) / sizeof(GattCharacteristic *));

//ISR for Button interrupt
void buttonIsr() {   

  button_char_value[0]++;
  
  ble.updateCharacteristicValue(button_characteristic.getValueAttribute().getHandle(), 
                                (const uint8_t*)button_char_value, 
                                1);
  
  Serial.print("Send data to Mobile via BLE: ");
  Serial.println(button_char_value[0]);                               
}


void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params) {  
  Serial.println("Disconnect");  
  digitalWrite(LED, LOW);         //off led    
  ble.startAdvertising();
}

void connectionCallback( const Gap::ConnectionCallbackParams_t *params ) {
  Serial.println("Connected");
}

void gattserverWriteCallback(const GattWriteCallbackParams *Handler) {
  
  static uint8_t buf[TXRX_BUF_LEN];
  static uint16_t bytes_read=0;

  if(Handler->handle == led_characteristic.getValueAttribute().getHandle()) {
           
    // Read the value of characteristic
    ble.readCharacteristicValue(led_characteristic.getValueAttribute().getHandle(), 
                                buf, 
                                &bytes_read);       
    if (bytes_read > 0) {                             
      //on LED
      if (buf[0] == 0x30) {
        digitalWrite(LED, HIGH);
        Serial.println("Turn LED on");
      }
      //off LED
      else {
        digitalWrite(LED, LOW);
        Serial.println("Turn LED off");
      }      
    }//if2
  }//if  
}

void setAdvertisement(void) {
  ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
  // Add short name to advertisement
  ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                   (const uint8_t *)"LB51", 
                                   4);
  
  // Add complete 128bit_uuid to advertisement
  ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                   (const uint8_t *)led_button_uuid, 
                                   sizeof(led_button_uuid));
   
  // Add complete device name to scan response data
  ble.accumulateScanResponse(GapAdvertisingData::COMPLETE_LOCAL_NAME,
                             (const uint8_t *)DEVICE_NAME, 
                             sizeof(DEVICE_NAME));
}

void setupBle(){

  // Init ble
  ble.init();
  ble.onConnection(connectionCallback);
  ble.onDisconnection(disconnectionCallback);
  ble.onDataWritten(gattserverWriteCallback);
  // set advertisement
  setAdvertisement();
  // set adv_type(enum from 0)
  ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);      
  // add service
  ble.addService(led_button_service);
  // set device name
  ble.setDeviceName((const uint8_t *)DEVICE_NAME);
  // set tx power,valid values are -40, -20, -16, -12, -8, -4, 0, 4
  ble.setTxPower(4);
  // set adv_interval, 100ms in multiples of 0.625ms.
  ble.setAdvertisingInterval(200);   
  // set adv_timeout, in seconds
  ble.setAdvertisingTimeout(0);
  // start advertising
  ble.startAdvertising();
}

void setup() {

  //init debug via serial  
  Serial.begin(115200);   
  Serial.println("Start application");

  //LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  //Button
  pinMode(BUT, INPUT);
  attachInterrupt(BUT, buttonIsr, FALLING);      //HIGH_TO_LOW signal
  
  //init BLE
  setupBle();
  
}

void loop() {
  //for BLE
  ble.waitForEvent();
}
