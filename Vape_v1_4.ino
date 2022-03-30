#include <LowPower.h>
#include <EEPROMex.h>
#include <Arduino.h>
#include <U8glib.h>
#include <TimerOne.h>

U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);

#define button_setting_pin 3
#define button_up_pin 4
#define button_down_pin 5
#define button_vape_pin 2
#define menu_items 5
#define metal_items 4
#define timer 100000
#define battery_voltage_min 3000
#define coil_temp_min 100
#define coil_temp_max 300
#define coil_resistance_min 500
#define coil_resistance_max 3000
#define font_height 13
#define vape_mosfet_pin 10
#define vape_check_mosfet_pin 8

volatile unsigned long butt_timer = millis();
unsigned long butt_hold_timer = 0;
unsigned long check_timer = 0;
unsigned long micro = 0;
long battery_voltage;
volatile byte menu = 0;
byte mode, metal;
byte i, h, w, d;
boolean butt_vape;
boolean butt_setting;
boolean butt_up;
boolean butt_down;
volatile boolean sleep_flag = 0;
byte menu_item;
byte metal_item;
volatile boolean butt_vape_flag = 1;
volatile boolean mosfet_vape_flag = 0;
boolean butt_setting_flag = 1;
boolean butt_up_flag = 1;
boolean butt_down_flag = 1;
byte butt_click = 0;
const char *menu_strings[menu_items] = { "VVolt", "VWatt", "VTemp", "COhms", "Metal" };
const char *metal_strings[metal_items] = { "Ni", "Ti", "SS", "MY" };
int metals[3] = { 6200, 3500, 920 };
char small_float[5];
int v_volt, v_volt_temp, coil_temp, coil_temp_temp, metal_custom, metal_custom_temp, coil_ohm, coil_ohm_temp, v_watt, v_watt_temp;
long volts_out, PWM;
int temperature_check, reference;
float normalR, dR;
const uint8_t rook_bitmap[] = {
  0x3c,
  0x7e,
  0xe7,
  0xc3,
  0xe7, 
  0x24,
  0xe7,
  0xe7
  };

void redraw() {    
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
}

void draw() {
  switch (menu) {
    case 0:
      u8g.setPrintPos(1, font_height);
      u8g.print("VOLTS");
      u8g.setPrintPos(4, font_height * 2);
      u8g.print(battery_voltage * 0.001);
      u8g.setPrintPos(4, font_height * 3);
      u8g.print("MODE");
      u8g.setPrintPos(1, font_height * 4);
      u8g.print(menu_strings[mode]);
      u8g.setPrintPos(1, font_height * 5);
      u8g.print("METAL");
      u8g.setPrintPos(10, font_height * 6);
      u8g.print(metal_strings[metal]);
      u8g.setPrintPos(4, font_height * 7);
      u8g.print(temperature_check);
      if (mosfet_vape_flag == 1) {
        u8g.setPrintPos(4, font_height * 9);
        u8g.print("VAPE");
        u8g.setPrintPos(4, 127);
        u8g.print(PWM);
      }
      break;
    case 1:
      u8g.drawStr(4, font_height, "MENU");
      for (i = 0; i < menu_items; i++)  {
        u8g.drawStr(1, ((font_height + 5) * i) + 30, menu_strings[i]);
        if (i == menu_item) u8g.drawRFrame(0, ((font_height + 5) * i) + 19, 32, font_height + 4, 2);
      }
      break;
    case 8:
      switch (butt_click) {
        case 0:
          u8g.print(" ");
          break;
        case 1:
          u8g.drawStr270(19, 87, "73");
          break;
        case 2:
          u8g.drawStr270(19, 87, "7355");
          break;
        case 3:
          u8g.drawStr270(19, 87, "735560");
          break;
        case 4:
          u8g.drawStr270(19, 87, "7355608");
          break;
      }
      break;
    case 9:
      u8g.drawStr270(19, 93, "LOW CHARGE");
      break;
    case 10:
      u8g.setPrintPos(1, 30);
      u8g.print("VVOLT");
      u8g.setPrintPos(1, 55);
      u8g.print((float)v_volt_temp * 0.001);
      u8g.print("V");
      break;
    case 11:
      u8g.setPrintPos(1, 30);
      u8g.print("VWATT");
      if (v_watt_temp > 99) u8g.setPrintPos(1, 55);
      else u8g.setPrintPos(4, 55);
      u8g.print(v_watt_temp);
      u8g.print(" W");
      break;
    case 12:
      u8g.setPrintPos(1, 30);
      u8g.print("VTEMP");
      u8g.setPrintPos(1, 55);
      u8g.print(coil_temp_temp);
      u8g.write(0xb0);
      u8g.print("C");
      break;
    case 13:
      u8g.setPrintPos(1, 30);
      u8g.print("COHMS");
      u8g.setPrintPos(0, 55);
      u8g.print((float)coil_ohm_temp * 0.001);
      u8g.drawBitmap(24, 47, 1, 8, rook_bitmap);
      break;
    case 14:
      u8g.setPrintPos(1, 30);
      u8g.print("METAL");
      for (i = 0; i < metal_items; i++)  {
        u8g.drawStr(10, ((font_height + 5) * i) + 50, metal_strings[i]);
        if (i == metal_item) u8g.drawRFrame(0, ((font_height + 5) * i) + 37, 32, font_height + 4, 4);
      }
      break;
    case 143:
      u8g.setPrintPos(4, 30);
      u8g.print("CUST");
      u8g.setPrintPos(1, 55);
      dtostrf(metal_custom_temp * 0.001, 5, 3, small_float);
      u8g.print(small_float);
      u8g.setPrintPos(1, 68);
      u8g.print("mOh/");
      u8g.write(0xb0);
      break;
  }
}

void sleep()  {
  Timer1.disablePwm(vape_mosfet_pin);
  digitalWrite(vape_mosfet_pin, LOW);
  mosfet_vape_flag = 0;
  digitalWrite(vape_check_mosfet_pin, LOW);
  attachInterrupt(digitalPinToInterrupt(button_vape_pin), wakeUp, FALLING);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  detachInterrupt(0);
}

void wakeUp() {
  menu = 8;
  butt_timer = millis();
  butt_hold_timer = millis();
  butt_vape_flag = 0;
  sleep_flag = 0;
  butt_click = 0;
  Timer1.disablePwm(vape_mosfet_pin);
  digitalWrite(vape_mosfet_pin, LOW);
  mosfet_vape_flag = 0;
  digitalWrite(vape_check_mosfet_pin, LOW);
}

/*
float voltageAverage(float newVoltage)  {
  static float currVoltage = analogRead(voltage_sensor_pin);
  if (abs(newVoltage - currVoltage) > 15.0) currVoltage += (newVoltage - currVoltage) * 0.9;
  else if (abs(newVoltage - currVoltage) > 5.0) currVoltage += (newVoltage - currVoltage) * 0.2;
  else currVoltage += (newVoltage - currVoltage) * 0.04;
  return currVoltage;
}
*/

long currentVoltage() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  micro = micros();
  while (micros() - micro <400) {
  }
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  uint8_t low_v  = ADCL;
  uint8_t high_v = ADCH;
  long result_v = (high_v << 8) | low_v;
  result_v = (1095.5 * 1023) / result_v;
  return result_v;
}

int currentTemperature() {
  ADMUX = _BV(REFS1) | _BV(REFS0);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  uint8_t low_t  = ADCL;
  uint8_t high_t = ADCH;
  int result_t = (high_t << 8) | low_t;
  return result_t;
}

void setup() {
  u8g.setRot270();
  pinMode(button_vape_pin, INPUT_PULLUP);
  pinMode(button_setting_pin, INPUT_PULLUP);
  pinMode(button_up_pin, INPUT_PULLUP);
  pinMode(button_down_pin, INPUT_PULLUP);
  pinMode(vape_mosfet_pin, OUTPUT);
  pinMode(vape_check_mosfet_pin, OUTPUT);
  Serial.begin(9600);  
  Timer1.initialize(64);
  Timer1.disablePwm(vape_mosfet_pin);
  digitalWrite(vape_mosfet_pin, LOW);
  mosfet_vape_flag = 0;
  digitalWrite(vape_check_mosfet_pin, LOW);
  u8g.setColorIndex(1);
/*  v_volt = 1500;
  v_watt = 15;
  coil_temp = 225;
  coil_ohm = 600;
  metal_custom = 17;
  normalR = 600;
  mode = 0;
  metal = 3;
  dR = 0.34;
  EEPROM.writeInt(0, v_volt);
  EEPROM.writeInt(2, v_watt);
  EEPROM.writeInt(6, coil_temp);
  EEPROM.writeInt(8, coil_ohm);
  EEPROM.writeInt(10, metal_custom);
  EEPROM.writeFloat(12, normalR);
  EEPROM.writeByte(16, mode);
  EEPROM.writeByte(17, metal);
  EEPROM.writeFloat(18, dR);*/
  v_volt = EEPROM.readInt(0);
  v_watt = EEPROM.readInt(2);
  coil_temp = EEPROM.readInt(6);
  coil_ohm = EEPROM.readInt(8);
  metal_custom = EEPROM.readInt(10);
  normalR = EEPROM.readFloat(12);
  mode = EEPROM.readByte(16);
  metal = EEPROM.readByte(17);
  dR = EEPROM.readFloat(18);
  u8g.setFont(u8g_font_6x13B);
  u8g.setFontRefHeightExtendedText();
  u8g.setFontPosBaseline();
  battery_voltage = currentVoltage();
}

void loop() {
  butt_vape = !digitalRead(button_vape_pin);
  butt_setting = !digitalRead(button_setting_pin);
  butt_up = !digitalRead(button_up_pin);
  butt_down = !digitalRead(button_down_pin);
  if (!butt_vape && butt_vape_flag && millis() - butt_timer > 50) {
    Timer1.disablePwm(vape_mosfet_pin);
    digitalWrite(vape_mosfet_pin, LOW);
    mosfet_vape_flag = 0;
    butt_vape_flag = 0;
  }
  else if (!butt_setting && butt_setting_flag && millis() - butt_timer > 50) {
    butt_setting_flag = 0;
  }
  else if (!butt_up && butt_up_flag && millis() - butt_timer > 50) {
    butt_up_flag = 0;
  }
  else if (!butt_down && butt_down_flag && millis() - butt_timer > 50) {
    butt_down_flag = 0;
  }
  if (butt_vape || butt_setting || butt_up || butt_down) butt_timer = millis();
  if (millis() - check_timer > 128) {
    if (!sleep_flag && menu == 0 && butt_vape && butt_vape_flag && !butt_setting && !butt_up && !butt_down && millis() - butt_hold_timer > 512 && millis() - butt_hold_timer < 5120) {
      butt_click = 0;
      switch(mode) {
        case 0:
          battery_voltage = currentVoltage();
          volts_out = min(v_volt, battery_voltage);
          PWM = volts_out * 1023 / battery_voltage;
          break;
        case 1:
          Timer1.disablePwm(vape_mosfet_pin);
          digitalWrite(vape_mosfet_pin, LOW);
          mosfet_vape_flag = 0;
          digitalWrite(vape_check_mosfet_pin, HIGH);
          micro = micros();
          while (micros() - micro < 500) {
          }
          temperature_check = currentTemperature();
          temperature_check += currentTemperature();
          temperature_check += currentTemperature();
          temperature_check /= 3;
          digitalWrite(vape_check_mosfet_pin, LOW);
          micro = micros();
          while (micros() - micro < 500) {
          }
          battery_voltage = currentVoltage();
          volts_out = sqrt(v_watt * 1000 * ((battery_voltage * 98 / temperature_check) - 1974));          // 1023/1095.5*R2
          if (volts_out < 0) volts_out = 0;
          volts_out = min(volts_out, battery_voltage);
          PWM = volts_out * 1023 / battery_voltage;
          break;
        case 2:
          Timer1.disablePwm(vape_mosfet_pin);
          digitalWrite(vape_mosfet_pin, LOW);
          mosfet_vape_flag = 0;
          digitalWrite(vape_check_mosfet_pin, HIGH);
          micro = micros();
          while (micros() - micro < 500) {
          }
          temperature_check = currentTemperature();
          temperature_check += currentTemperature();
          temperature_check += currentTemperature();
          temperature_check /= 3;
          digitalWrite(vape_check_mosfet_pin, LOW);
          micro = micros();
          while (micros() - micro < 500) {
          }
          battery_voltage = currentVoltage();
          volts_out = battery_voltage * (float)(1.0 - (((((float)battery_voltage * 98.0 / (float)temperature_check) - 1974.0) - normalR) / dR));
          if (volts_out < 0) volts_out = 0;
          volts_out = min(volts_out, battery_voltage);
          PWM = volts_out * 1023 / battery_voltage;
          break;
      }
      Timer1.pwm(vape_mosfet_pin, PWM);
      mosfet_vape_flag = 1;
    }
    else {
      battery_voltage = currentVoltage();
      micro = micros();
      while (micros() - micro < 1000) {
      }
    }
    check_timer = millis();
  }
  if (millis() - butt_timer > timer) {
    butt_click = 0;
    menu = 8;
    redraw();
    sleep();
  }
  else if (battery_voltage <= battery_voltage_min) {
    if (millis() - butt_timer < 3072) {
      menu = 9;
      Timer1.disablePwm(vape_mosfet_pin);
      digitalWrite(vape_mosfet_pin, LOW);
      mosfet_vape_flag = 0;
      digitalWrite(vape_check_mosfet_pin, LOW);
      redraw();
    }
    if (millis() - butt_timer > 3072) {
      butt_click = 0;
      menu = 8;
      redraw();
      sleep();
    }
  }
  else {
    switch (menu) {
      case 0:
        if (!sleep_flag) {
          if (millis() - butt_timer > 1024) butt_click = 0;
          if (butt_setting && !butt_setting_flag && !butt_vape && !butt_up && !butt_down) {
            butt_setting_flag = 1;
            menu = 1;
            menu_item = 0;
            redraw();
          }
          if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
            butt_vape_flag = 1;
            butt_hold_timer = millis();
            if (millis() - butt_timer < 1024) {
              butt_click++;
            }
            if (butt_click >= 5) {
              butt_click = 0;
              menu = 8;
              redraw();
              menu = 0;
              sleep_flag = 1;
            }
            redraw;
          }
          else if (butt_vape && butt_vape_flag && !butt_setting && !butt_up && !butt_down && millis() - butt_hold_timer >= 5120) {
            Timer1.disablePwm(vape_mosfet_pin);
            digitalWrite(vape_mosfet_pin, LOW);
            digitalWrite(vape_check_mosfet_pin, LOW);
            mosfet_vape_flag = 0;
          }
          else redraw();;
        }
        else if (sleep_flag && millis() - butt_timer > 64) {
          sleep();
        }
        break;
      case 1:
        if (butt_setting && !butt_setting_flag && !butt_vape && !butt_up && !butt_down) {
          butt_setting_flag = 1;
          menu = 0; 
          redraw();
        }
        if (butt_up && !butt_up_flag && !butt_vape && !butt_setting && !butt_down) {
          butt_up_flag = 1;
          if (menu_item == 0) menu_item = menu_items;
          menu_item--; 
          redraw();
        }
        if (butt_down && !butt_down_flag && !butt_vape && !butt_up && !butt_setting) {
          butt_down_flag = 1;
          menu_item++; 
          if(menu_item == menu_items) menu_item = 0;
          redraw();
        }
        if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
          butt_vape_flag = 1;
          menu = (menu * 10) + menu_item;
          switch (menu_item) {
            case 0:
              v_volt = EEPROM.readInt(0);
              v_volt_temp = v_volt;
              break;
            case 1:
              v_watt = EEPROM.readInt(2);
              v_watt_temp = v_watt;
              break;
            case 2:
              coil_temp = EEPROM.readInt(6);
              coil_temp_temp = coil_temp + 25;
              break;
            case 3:
              coil_ohm = EEPROM.readInt(8);
              coil_ohm_temp = coil_ohm - 100;
              break;
            case 4:
              metal_item = 0;
              metal_custom = EEPROM.readInt(10);
              metal_custom_temp = metal_custom;
              break;
          }
          redraw();
        }
        break;
      case 8:
        if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
          butt_vape_flag = 1;
          if (millis() - butt_timer < 1024) {
            butt_click++;
            redraw();
          }
          if (butt_click >= 5) {
            menu = 0;
            butt_click = 1;
            butt_hold_timer = millis();
          }
          redraw;
        }
        if (millis() - butt_timer > 1024) {
          butt_click = 0;
          redraw();
          menu = 0;
          sleep_flag = 1;
        }
        break;
      case 10:
        if (butt_setting  && !butt_setting_flag && !butt_vape && !butt_up && !butt_down) {
          butt_setting_flag = 1;
          menu = menu / 10;
          redraw();
        }
        if (butt_up && !butt_up_flag && !butt_vape && !butt_setting && !butt_down) {
          butt_up_flag = 1;
          butt_hold_timer = millis();
          v_volt_temp += 10;
          v_volt_temp = min(v_volt_temp, battery_voltage);
          redraw();
        }
        else if (butt_up && butt_up_flag && !butt_setting && !butt_vape && !butt_down && millis() - butt_hold_timer > 1024) {
          v_volt_temp += 10;
          v_volt_temp = min(v_volt_temp, battery_voltage);
          redraw();
        }
        if (butt_down && !butt_down_flag && !butt_vape && !butt_up && !butt_setting) {
          butt_down_flag = 1;
          butt_hold_timer = millis();
          v_volt_temp -= 10;
          v_volt_temp = max(v_volt_temp, 500);
          redraw();
        }
        else if (butt_down && butt_down_flag && !butt_setting && !butt_vape && !butt_up && millis() - butt_hold_timer > 1024) {
          v_volt_temp -= 10;
          v_volt_temp = max(v_volt_temp, 500);
          redraw();
        }
        if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
          butt_vape_flag = 1;
          mode = 0;
          EEPROM.updateByte(16, mode);
          menu = menu / 10;
          v_volt = v_volt_temp;
          EEPROM.updateInt(0, v_volt);
          redraw();
        }
        break;
      case 11:
        if (butt_setting  && !butt_setting_flag && !butt_vape && !butt_up && !butt_down) {
          butt_setting_flag = 1;
          menu = menu / 10;
          redraw();
        }
        if (butt_up && !butt_up_flag && !butt_vape && !butt_setting && !butt_down) {
          butt_up_flag = 1;
          butt_hold_timer = millis();
          v_watt_temp += 1;
          v_watt_temp = min(v_watt_temp, ((battery_voltage * battery_voltage) / coil_ohm) / 1000);
          redraw();
        }
        else if (butt_up && butt_up_flag && !butt_setting && !butt_vape && !butt_down && millis() - butt_hold_timer > 1024) {
          v_watt_temp += 1;
          v_watt_temp = min(v_watt_temp, ((battery_voltage * battery_voltage) / coil_ohm) / 1000);
          redraw();
        }
        if (butt_down && !butt_down_flag && !butt_vape && !butt_up && !butt_setting) {
          butt_down_flag = 1;
          butt_hold_timer = millis();
          v_watt_temp -= 1;
          v_watt_temp = max(v_watt_temp, 2);
          redraw();
        }
        else if (butt_down && butt_down_flag && !butt_setting && !butt_vape && !butt_up && millis() - butt_hold_timer > 1024) {
          v_watt_temp -= 1;
          v_watt_temp = max(v_watt_temp, 2);
          redraw();
        }
        if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
          butt_vape_flag = 1;
          mode = 1;
          EEPROM.updateByte(16, mode);
          menu = menu / 10;
          v_watt = v_watt_temp;
          EEPROM.updateInt(2, v_watt);
          redraw();
        }
        break;
      case 12:
        if (butt_setting  && !butt_setting_flag && !butt_vape && !butt_up && !butt_down) {
          butt_setting_flag = 1;
          menu = menu / 10;
          redraw();
        }
        if (butt_up && !butt_up_flag && !butt_vape && !butt_setting && !butt_down) {
          butt_up_flag = 1;
          butt_hold_timer = millis();
          coil_temp_temp += 1;
          coil_temp_temp = min(coil_temp_temp, coil_temp_max);
          redraw();
        }
        else if (butt_up && butt_up_flag && !butt_setting && !butt_vape && !butt_down && millis() - butt_hold_timer > 1024) {
          coil_temp_temp += 1;
          coil_temp_temp = min(coil_temp_temp, coil_temp_max);
          redraw();
        }
        if (butt_down && !butt_down_flag && !butt_vape && !butt_up && !butt_setting) {
          butt_down_flag = 1;
          butt_hold_timer = millis();
          coil_temp_temp -= 1;
          coil_temp_temp = max(coil_temp_temp, coil_temp_min);
          redraw();
        }
        else if (butt_down && butt_down_flag && !butt_setting && !butt_vape && !butt_up && millis() - butt_hold_timer > 1024) {
          coil_temp_temp -= 1;
          coil_temp_temp = max(coil_temp_temp, coil_temp_min);
          redraw();
        }
        if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
          butt_vape_flag = 1;
          mode = 2;
          EEPROM.updateByte(16, mode);
          menu = menu / 10;
          coil_temp = coil_temp_temp - 25;
          if (metal == 3) dR = normalR * ((float)coil_temp * metal_custom * 0.000001);
          else dR = normalR * ((float)coil_temp * metals[metal] * 0.000001);
          EEPROM.updateInt(6, coil_temp);
          EEPROM.updateFloat(18, dR);
          redraw();
        }
        break;
      case 13:
        if (butt_setting  && !butt_setting_flag && !butt_vape && !butt_up && !butt_down) {
          butt_setting_flag = 1;
          menu = menu / 10;
          redraw();
        }
        if (butt_up && !butt_up_flag && !butt_vape && !butt_setting && !butt_down) {
          butt_up_flag = 1;
          butt_hold_timer = millis();
          coil_ohm_temp += 10;
          coil_ohm_temp = min(coil_ohm_temp, coil_resistance_max);
          redraw();
        }
        else if (butt_up && butt_up_flag && !butt_setting && !butt_vape && !butt_down && millis() - butt_hold_timer > 1024) {
          coil_ohm_temp += 10;
          coil_ohm_temp = min(coil_ohm_temp, coil_resistance_max);
          redraw();
        }
        if (butt_down && !butt_down_flag && !butt_vape && !butt_up && !butt_setting) {
          butt_down_flag = 1;
          butt_hold_timer = millis();
          coil_ohm_temp -= 10;
          coil_ohm_temp = max(coil_ohm_temp, coil_resistance_min);
          redraw();
        }
        else if (butt_down && butt_down_flag && !butt_setting && !butt_vape && !butt_up && millis() - butt_hold_timer > 1024) {
          coil_ohm_temp -= 10;
          coil_ohm_temp = max(coil_ohm_temp, coil_resistance_min);
          redraw();
        }
        if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
          butt_vape_flag = 1;
          menu = menu / 10;
          coil_ohm = coil_ohm_temp + 100;
          EEPROM.updateInt(8, coil_ohm);
          reference = 0;
          Timer1.disablePwm(vape_mosfet_pin);
          digitalWrite(vape_mosfet_pin, LOW);
          mosfet_vape_flag = 0;
          digitalWrite(vape_check_mosfet_pin, HIGH);
          micro = micros();
          while (micros() - micro < 500) {
          }
          for (i = 0; i < 15; i++) {
            reference += currentTemperature();
          }
          digitalWrite(vape_check_mosfet_pin, LOW);
          while (micros() - micro < 500) {
          }
          reference /= 15;
          normalR = (float)((battery_voltage * 98.051 / reference) - 1974);
          EEPROM.updateFloat(12, normalR);
          redraw();
        }
        break;
      case 14:
        if (butt_setting && !butt_setting_flag && !butt_vape && !butt_up && !butt_down) {
          butt_setting_flag = 1;
          menu = menu / 10;
          redraw();
        }
        if (butt_up && !butt_up_flag && !butt_vape && !butt_setting && !butt_down) {
          butt_up_flag = 1;
          if (metal_item == 0) metal_item = metal_items;
          metal_item--; 
          redraw();
        }
        if (butt_down && !butt_down_flag && !butt_vape && !butt_up && !butt_setting) {
          butt_down_flag = 1;
          metal_item++; 
          if(metal_item == metal_items) metal_item = 0;
          redraw();
        }
        if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
          butt_vape_flag = 1;
          if (metal_item == metal_items - 1) {
            menu = (menu * 10) + metal_item;
          }
          else {
            metal = metal_item;
            dR = normalR * ((float)coil_temp * metals[metal] * 0.000001);
            EEPROM.updateByte(17, metal);
            EEPROM.updateFloat(18, dR);
            menu = menu / 10;
          }
          redraw();
        }
        break;
      case 143:
        if (butt_setting  && !butt_setting_flag && !butt_vape && !butt_up && !butt_down) {
          butt_setting_flag = 1;
          menu = menu / 10;
          redraw();
        }
        if (butt_up && !butt_up_flag && !butt_vape && !butt_setting && !butt_down) {
          butt_up_flag = 1;
          butt_hold_timer = millis();
          metal_custom_temp += 1;
          redraw();
        }
        else if (butt_up && butt_up_flag && !butt_setting && !butt_vape && !butt_down && millis() - butt_hold_timer > 1024) {
          metal_custom_temp += 1;
          redraw();
        }
        if (butt_down && !butt_down_flag && !butt_vape && !butt_up && !butt_setting) {
          butt_down_flag = 1;
          butt_hold_timer = millis();
          metal_custom_temp -= 1;
          redraw();
        }
        else if (butt_down && butt_down_flag && !butt_setting && !butt_vape && !butt_up && millis() - butt_hold_timer > 1024) {
          metal_custom_temp += 1;
          redraw();
        }
        if (butt_vape && !butt_vape_flag && !butt_setting && !butt_up && !butt_down) {
          butt_vape_flag = 1;
          menu = menu / 10;
          metal = metal_item;
          EEPROM.updateByte(17, metal);
          metal_custom = metal_custom_temp;
          dR = normalR * ((float)coil_temp * metal_custom * 0.000001);
          EEPROM.updateInt(10, metal_custom);
          EEPROM.updateFloat(18, dR);
          redraw();
        }
        break;
    }
  }
}
