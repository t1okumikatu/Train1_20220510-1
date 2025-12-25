/* broadcast_Train1 control
 * 2021/09/16  Ver1.00 初版
 * 2021/09/16  Ver1.01 watchdog timer対応OK
 * 2021/09/17  Ver1.50 実機対応修正(１号機用)
 * 2021/09/20  Ver1.51 走行、停止対応
 * 2021/09/26  Ver1.52 送信機能停止
 * 2022/02/10  Ver1.53 モーターPWM周波数、Bit変更
 * 2022/03/01  Ver2.00 LED制御追加
 * 2022/05/10  Ver3.00 台形制御
 *                     JROBO junichi itoh
 *             
 */
#include <esp_now.h>
#include <WiFi.h>

#define FrontLight_R 26  // 前方灯（赤）
#define FrontLight_B 25  // 前方灯（青）
#define BackLight_B 32   // 後方灯（青）
#define BackLight_R 33   // 後方灯（赤）
#define LoomLight1 21    // 室内灯1
#define LoomLight2 19    // 室内灯2
#define LoomLight3 18    // 室内灯3
#define LoomLight4 4     // 室内灯4

int Train1Speed;
int Speed;
int SetSpeed;
int FB_Light;
unsigned long SetTime1;

//const int IN1 = 23;
//const int IN2 = 22;
const int IN1 = 22;
const int IN2 = 23;
const int CHANNEL_0 = 0;
const int CHANNEL_1 = 1;
//const int LEDC_TIMER_BIT = 10;   // PWMの範囲(8bitなら0〜255、10bitなら0〜1023)
const int LEDC_TIMER_BIT = 9;    // PWMの範囲(8bitなら0〜255、9Bitなら0～512, 10bitなら0〜1023)
const int LEDC_BASE_FREQ = 100;  // 周波数(Hz)
const int VALUE_MAX = 500;       // PWMの最大値
//const int VALUE_MAX = 1000;      // PWMの最大値
uint8_t data[9] = { 98, 0, 0, 0, 0, 0, 0, 0, 0 };
esp_now_peer_info_t slave;

// ** 送信コールバック *********************************************************************
//void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//  char macStr[18];
//  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
//           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//}
// ***************************************************************************************


// ** 受信コールバック *********************************************************************
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //  for ( int i = 0 ; i < data_len ; i++ ) {
  if (data[0] == 99) {
    Train1Speed = data[1];  // 1号車走行速度指令
    FB_Light = data[2];     // 1号車前後走行指令、前照灯,ルーム照明
  }
}
// ***************************************************************************************


// ** setup ******************************************************************************
void setup() {
  Serial.begin(115200);
  pinMode(LoomLight1, OUTPUT);
  pinMode(LoomLight2, OUTPUT);
  pinMode(LoomLight3, OUTPUT);
  pinMode(LoomLight4, OUTPUT);
  pinMode(FrontLight_B, OUTPUT);
  pinMode(FrontLight_R, OUTPUT);
  pinMode(BackLight_B, OUTPUT);
  pinMode(BackLight_R, OUTPUT);

  digitalWrite(FrontLight_B, LOW);
  digitalWrite(FrontLight_R, LOW);
  digitalWrite(BackLight_B, LOW);
  digitalWrite(BackLight_R, LOW);
  digitalWrite(LoomLight1, LOW);
  digitalWrite(LoomLight2, LOW);
  digitalWrite(LoomLight3, LOW);
  digitalWrite(LoomLight4, LOW);

  // ** ESP-NOW初期化 *********************************************************************
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
  } else {
    return;
  }
  // ***************************************************************************************
  // ** マルチキャスト用Slave登録 ************************************************************
  memset(&slave, 0, sizeof(slave));

  for (int i = 0; i < 6; ++i) {
    slave.peer_addr[i] = (uint8_t)0xff;
  }

  esp_err_t addStatus = esp_now_add_peer(&slave);
  if (addStatus == ESP_OK) {
    //    digitalWrite(LoomLight1, HIGH);                // WiFi確定でLoomLight点灯
  }
  // ****************************************************************************************
  // ** ESP-NOWコールバック登録 **************************************************************
  //esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  // ****************************************************************************************
  disableCore0WDT();  // watchdog timer対応
  disableCore1WDT();  // watchdog timer対応

  pinMode(IN1, OUTPUT);  // IN1
  pinMode(IN2, OUTPUT);  // IN2

  ledcSetup(CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
  ledcSetup(CHANNEL_1, LEDC_BASE_FREQ, LEDC_TIMER_BIT);

  ledcAttachPin(IN1, CHANNEL_0);
  ledcAttachPin(IN2, CHANNEL_1);
  /*
  if(FB_Light == 99){    // イニシャルモード表示（LED５回点滅）
    for(int i = 0; i > 5; i++){
      digitalWrite(FrontLight_B, HIGH);
      digitalWrite(BackLight_B,  HIGH);
      delay(500);
      digitalWrite(FrontLight_B, LOW);
      digitalWrite(BackLight_B,  LOW);
    }
    for(int i = 0; i > 5; i++){
      digitalWrite(FrontLight_R, HIGH);
      digitalWrite(BackLight_R,  HIGH);
      delay(500);
      digitalWrite(FrontLight_R, LOW);
      digitalWrite(BackLight_R,  LOW);
    }
  }
  digitalWrite(FrontLight_R, HIGH);       // 前後照明（赤）点灯よりスタート
  digitalWrite(BackLight_R,  HIGH);
*/
}
// ****************************************************************************************

// ** Loop ********************************************************************************
void loop() {
  Serial.print("Speed : ");
  Serial.print(Train1Speed);
  Serial.print("  Speed : ");
  Serial.print(Speed);
  Serial.print("  Speed : ");
  Serial.print(SetSpeed);
  Serial.print("   Commnd : ");
  Serial.println(FB_Light);
  //  esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));
  // ** Speed : ( 0 - 255) *　3倍 **********************************************************
  if (Train1Speed == 0) {
    SetSpeed = 0;
    //    brake();
  }
  if (Train1Speed != 0) {
    Speed = Train1Speed * 3;
    SetTime1 = millis();
    if (millis() > (SetTime1 + 100)) {
      SetSpeed = SetSpeed + 1;
      if (SetSpeed >= Speed) {
        SetSpeed = Speed;
      }
    }
  }

  // ** Forward ****************
  if (FB_Light == 10) {
    brake();
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, LOW);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, LOW);
  }
  if (FB_Light == 11) {
    brake();
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, LOW);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, HIGH);
  }
  if (FB_Light == 12) {
    forward(SetSpeed);
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, HIGH);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, HIGH);
  }
  if (FB_Light == 13) {
    forward(SetSpeed);
    digitalWrite(FrontLight_B, HIGH);
    digitalWrite(FrontLight_R, LOW);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, HIGH);
  }
  if (FB_Light == 14) {
    forward(Speed);
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, LOW);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, HIGH);
  }
  if (FB_Light == 15) {
    forward(SetSpeed);
  }
  if (FB_Light == 16) {
    forward(SetSpeed);
  }
  if (FB_Light == 17) {
    forward(SetSpeed);
  }
  if (FB_Light == 18) {
    forward(SetSpeed);
  }
  if (FB_Light == 19) {
    forward(SetSpeed);
  }
  // ** Reverse ****************
  if (FB_Light == 20) {
    brake();
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, LOW);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, LOW);
  }
  if (FB_Light == 21) {
    brake();
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, HIGH);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, LOW);
  }
  if (FB_Light == 22) {
    reverse(SetSpeed);
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, HIGH);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, HIGH);
  }
  if (FB_Light == 23) {
    reverse(SetSpeed);
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, HIGH);
    digitalWrite(BackLight_B, HIGH);
    digitalWrite(BackLight_R, LOW);
  }
  if (FB_Light == 24) {
    reverse(SetSpeed);
    digitalWrite(FrontLight_B, LOW);
    digitalWrite(FrontLight_R, HIGH);
    digitalWrite(BackLight_B, LOW);
    digitalWrite(BackLight_R, LOW);
  }
  if (FB_Light == 25) {
    reverse(SetSpeed);
  }
  if (FB_Light == 26) {
    reverse(SetSpeed);
  }
  if (FB_Light == 27) {
    reverse(SetSpeed);
  }
  if (FB_Light == 28) {
    reverse(SetSpeed);
  }
  if (FB_Light == 29) {
    reverse(SetSpeed);
  }
  // **************************************************************************************
  delay(1);  // watchdog timer対応
}
// ****************************************************************************************

// ** Moter Contlor ***********************************************************************
// ** 正転 *********************************************************************************
void forward(uint32_t pwm) {
  if (pwm > VALUE_MAX) {
    pwm = VALUE_MAX;
  }
  ledcWrite(CHANNEL_0, pwm);
  ledcWrite(CHANNEL_1, 0);
}

// ** 逆転 *********************************************************************************
void reverse(uint32_t pwm) {
  if (pwm > VALUE_MAX) {
    pwm = VALUE_MAX;
  }
  ledcWrite(CHANNEL_0, 0);
  ledcWrite(CHANNEL_1, pwm);
}

// ** ブレーキ ******************************************************************************
void brake() {
  ledcWrite(CHANNEL_0, VALUE_MAX);
  ledcWrite(CHANNEL_1, VALUE_MAX);
}

// ** 空転 **********************************************************************************
void coast() {
  ledcWrite(CHANNEL_0, 0);
  ledcWrite(CHANNEL_1, 0);
}
// ******************************************************************************************