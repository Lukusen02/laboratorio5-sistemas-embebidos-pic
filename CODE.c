#include <16F877A.h>
#fuses XT,NOWDT,NOPROTECT,NOLVP
#use delay(clock=4000000)
#use fast_io(A)
#use fast_io(B)
#use fast_io(C)
#use fast_io(D)

// --- MASTER DELAY ------------------------------------------------------------
#define MASTER_DELAY 1

// --- PIN DEFINITIONS ---------------------------------------------------------
#define LCD_RS  PIN_D0
#define LCD_E   PIN_D1
#define LCD_D4  PIN_D4
#define LCD_D5  PIN_D5
#define LCD_D6  PIN_D6
#define LCD_D7  PIN_D7

#define BTN_LEFT    PIN_B0
#define BTN_RIGHT   PIN_B1
#define BTN_SELECT  PIN_B2

#define LED1  PIN_A0
#define LED2  PIN_A1
#define LED3  PIN_A2
#define LED4  PIN_A3
#define LED5  PIN_A5


#define PUMP      PIN_C0
#define STEP_DIR  PIN_C1
#define STEP_PUL  PIN_C2
#define LCD_BL PIN_C3

// --- CONSTANTS ---------------------------------------------------------------
#define STATE_IDLE    0
#define STATE_MENU    1
#define STATE_SUBMENU 2

#define MENU_BOMBA  0
#define MENU_MOTOR  1
#define MENU_LEDS   2
#define MENU_TEXTO  3
#define MENU_COUNT  4

#define TIMEOUT_TICKS 30000

// --- GLOBAL STATE -------------------------------------------------------------
int8 sys_state    = STATE_IDLE;
int8 menu_idx     = 0;
int8 sub_idx      = 0;

int8  pump_mode   = 0;
int8  motor_mode  = 0;
int8  led_mode    = 0;
int8  text_speed  = 5;   // 0=lento … 10=rápido

int16 pump_cnt    = 0;
int16 motor_cnt   = 0;
int16 led_cnt     = 0;
int8  led_pos     = 0;
int16 scroll_cnt  = 0;
int8  scroll_pos  = 0;
int16 timeout_cnt = 0;
int8  pump_state  = 0;
int8  step_state  = 0;

// --- SCROLL TEXT -------------------------------------------------------------
int8 scroll_text[80];
int8 scroll_len = 0;

void build_scroll_text(void) {
   int8 i = 0;
   scroll_text[i++]='L'; scroll_text[i++]='a'; scroll_text[i++]='b';
   scroll_text[i++]='o'; scroll_text[i++]='r'; scroll_text[i++]='a';
   scroll_text[i++]='t'; scroll_text[i++]='o'; scroll_text[i++]='r';
   scroll_text[i++]='i'; scroll_text[i++]='o'; scroll_text[i++]=' ';
   scroll_text[i++]='#'; scroll_text[i++]='5'; scroll_text[i++]=' ';
   scroll_text[i++]='U'; scroll_text[i++]='n'; scroll_text[i++]='i';
   scroll_text[i++]='v'; scroll_text[i++]='e'; scroll_text[i++]='r';
   scroll_text[i++]='s'; scroll_text[i++]='i'; scroll_text[i++]='d';
   scroll_text[i++]='a'; scroll_text[i++]='d'; scroll_text[i++]=' ';
   scroll_text[i++]='P'; scroll_text[i++]='o'; scroll_text[i++]='n';
   scroll_text[i++]='t'; scroll_text[i++]='i'; scroll_text[i++]='f';
   scroll_text[i++]='i'; scroll_text[i++]='c'; scroll_text[i++]='i';
   scroll_text[i++]='a'; scroll_text[i++]=' '; scroll_text[i++]='B';
   scroll_text[i++]='o'; scroll_text[i++]='l'; scroll_text[i++]='i';
   scroll_text[i++]='v'; scroll_text[i++]='a'; scroll_text[i++]='r';
   scroll_text[i++]='i'; scroll_text[i++]='a'; scroll_text[i++]='n';
   scroll_text[i++]='a'; scroll_text[i++]=' '; scroll_text[i++]='J';
   scroll_text[i++]='u'; scroll_text[i++]='a'; scroll_text[i++]='n';
   scroll_text[i++]=' '; scroll_text[i++]='P'; scroll_text[i++]='a';
   scroll_text[i++]='b'; scroll_text[i++]='l'; scroll_text[i++]='o';
   scroll_text[i++]=' '; scroll_text[i++]='y'; scroll_text[i++]=' ';
   scroll_text[i++]='S'; scroll_text[i++]='a'; scroll_text[i++]='m';
   scroll_text[i++]='u'; scroll_text[i++]='e'; scroll_text[i++]='l';
   scroll_len = i;
}

// --- SCROLL PERIOD (depende de text_speed) -----------------------------------
// Retorna el número de ticks de 1 ms entre cada paso de scroll.
// Rango visible en Proteus: 800 ms (muy lento) … 2 ms (muy rápido).
int16 get_scroll_period(void) {
   if      (text_speed == 0)  return 800;
   else if (text_speed == 1)  return 600;
   else if (text_speed == 2)  return 400;
   else if (text_speed == 3)  return 250;
   else if (text_speed == 4)  return 150;
   else if (text_speed == 5)  return 80;
   else if (text_speed == 6)  return 40;
   else if (text_speed == 7)  return 20;
   else if (text_speed == 8)  return 10;
   else if (text_speed == 9)  return 5;
   else                       return 2;   // speed == 10
}

// --- LCD LOW-LEVEL ------------------------------------------------------------
void lcd_pulse_e(void) {
   output_high(LCD_E);
   delay_us(2 * MASTER_DELAY);
   output_low(LCD_E);
   delay_us(2 * MASTER_DELAY);
}

void lcd_nibble(int8 n) {
   if (n & 0x01) output_high(LCD_D4); else output_low(LCD_D4);
   if (n & 0x02) output_high(LCD_D5); else output_low(LCD_D5);
   if (n & 0x04) output_high(LCD_D6); else output_low(LCD_D6);
   if (n & 0x08) output_high(LCD_D7); else output_low(LCD_D7);
   lcd_pulse_e();
}

void lcd_send(int8 data, int1 rs) {
   if (rs) output_high(LCD_RS); else output_low(LCD_RS);
   lcd_nibble(data >> 4);
   lcd_nibble(data & 0x0F);
   delay_us(50 * MASTER_DELAY);
}

void lcd_cmd(int8 cmd) { lcd_send(cmd, 0); }
void lcd_char(int8 c)  { lcd_send(c,   1); }

void lcd_goto(int8 row, int8 col) {
   int8 addr;
   if (row == 0) addr = 0x80 + col;
   else          addr = 0xC0 + col;
   lcd_cmd(addr);
}

void lcd_clear(void) {
   lcd_cmd(0x01);
   delay_ms(2 * MASTER_DELAY);
}

void lcd_init(void) {
   delay_ms(20 * MASTER_DELAY);
   output_low(LCD_RS);
   output_low(LCD_E);
   lcd_nibble(0x03); delay_ms(5 * MASTER_DELAY);
   lcd_nibble(0x03); delay_ms(1 * MASTER_DELAY);
   lcd_nibble(0x03); delay_ms(1 * MASTER_DELAY);
   lcd_nibble(0x02); delay_ms(1 * MASTER_DELAY);
   lcd_cmd(0x28);
   lcd_cmd(0x0C);
   lcd_cmd(0x06);
   lcd_clear();
}

void lcd_digit(int8 d) { lcd_char('0' + d); }

// --- STRING PRINTERS ---------------------------------------------------------
void lcd_print_bomba(void)     { lcd_char('B');lcd_char('o');lcd_char('m');lcd_char('b');lcd_char('a'); }
void lcd_print_motor(void)     { lcd_char('M');lcd_char('o');lcd_char('t');lcd_char('o');lcd_char('r'); }
void lcd_print_leds(void)      { lcd_char('L');lcd_char('E');lcd_char('D');lcd_char('s'); }
void lcd_print_texto(void)     { lcd_char('T');lcd_char('e');lcd_char('x');lcd_char('t');lcd_char('o'); }
void lcd_print_vel1(void)      { lcd_char('V');lcd_char('e');lcd_char('l');lcd_char('1'); }
void lcd_print_vel2(void)      { lcd_char('V');lcd_char('e');lcd_char('l');lcd_char('2'); }
void lcd_print_vel3(void)      { lcd_char('V');lcd_char('e');lcd_char('l');lcd_char('3'); }
void lcd_print_stop(void)      { lcd_char('S');lcd_char('t');lcd_char('o');lcd_char('p'); }
void lcd_print_derecha(void)   { lcd_char('D');lcd_char('e');lcd_char('r');lcd_char('e');lcd_char('c');lcd_char('h');lcd_char('a'); }
void lcd_print_izquierda(void) { lcd_char('I');lcd_char('z');lcd_char('q');lcd_char('u');lcd_char('i');lcd_char('e');lcd_char('r');lcd_char('d');lcd_char('a'); }
void lcd_print_lrd(void)       { lcd_char('I');lcd_char('z');lcd_char('q');lcd_char('-');lcd_char('>');lcd_char('D');lcd_char('e');lcd_char('r'); }
void lcd_print_drl(void)       { lcd_char('D');lcd_char('e');lcd_char('r');lcd_char('-');lcd_char('>');lcd_char('I');lcd_char('z');lcd_char('q'); }
void lcd_print_speed(void)     { lcd_char('S');lcd_char('p');lcd_char('d');lcd_char(':'); }

void lcd_spaces(int8 n) {
   int8 k;
   for (k = 0; k < n; k++) lcd_char(' ');
}

// --- MENÚ: nombres y longitudes ----------------------------------------------
int8 menu_name_len(int8 idx) {
   if (idx == MENU_BOMBA) return 5;
   if (idx == MENU_MOTOR) return 5;
   if (idx == MENU_LEDS)  return 4;
   return 5;
}

void lcd_print_menu_name(int8 idx) {
   if      (idx == MENU_BOMBA) lcd_print_bomba();
   else if (idx == MENU_MOTOR) lcd_print_motor();
   else if (idx == MENU_LEDS)  lcd_print_leds();
   else                        lcd_print_texto();
}

void lcd_show_menu_title(void) {
   lcd_goto(0, 0);
   lcd_spaces(6);
   lcd_char('M'); lcd_char('E'); lcd_char('N'); lcd_char('U');
   lcd_spaces(6);
}

void lcd_show_menu_option(int8 idx) {
   int8 nlen  = menu_name_len(idx);
   int8 total = nlen + 4;
   int8 pad_l = (16 - total) / 2;
   int8 pad_r = 16 - total - pad_l;
   lcd_goto(1, 0);
   lcd_spaces(pad_l);
   lcd_char('<'); lcd_char(' ');
   lcd_print_menu_name(idx);
   lcd_char(' '); lcd_char('>');
   lcd_spaces(pad_r);
}

void show_main_menu(void) {
   lcd_clear();
   lcd_show_menu_title();
   lcd_show_menu_option(menu_idx);
}

// --- SUBMENÚ -----------------------------------------------------------------
int8 sub_max(void) {
   if (menu_idx == MENU_BOMBA) return 4;
   if (menu_idx == MENU_MOTOR) return 3;
   if (menu_idx == MENU_LEDS)  return 3;
   return 13;
}

int8 sub_option_len(int8 m, int8 idx) {
   if (m == MENU_BOMBA) return 4;
   if (m == MENU_MOTOR) {
      if (idx == 0) return 7;
      if (idx == 1) return 9;
      return 4;
   }
   if (m == MENU_LEDS) {
      if (idx < 2) return 8;
      return 4;
   }
   if (idx == 10) return 6;
   return 5;
}

void print_sub_option_text(int8 m, int8 idx) {
   if (m == MENU_BOMBA) {
      if      (idx == 0) lcd_print_vel1();
      else if (idx == 1) lcd_print_vel2();
      else if (idx == 2) lcd_print_vel3();
      else               lcd_print_stop();
   } else if (m == MENU_MOTOR) {
      if      (idx == 0) lcd_print_derecha();
      else if (idx == 1) lcd_print_izquierda();
      else               lcd_print_stop();
   } else if (m == MENU_LEDS) {
      if      (idx == 0) lcd_print_lrd();
      else if (idx == 1) lcd_print_drl();
      else               lcd_print_stop();
   } else {
      lcd_print_speed();
      if (idx <= 10) {
   lcd_print_speed();
   if (idx == 10) { lcd_char('1'); lcd_char('0'); }
   else           { lcd_digit(idx); }
}
else if (idx == 11) {
   lcd_char('L');lcd_char('E');lcd_char('D');
   lcd_char(' ');lcd_char('O');lcd_char('N');
}
else {
   lcd_char('L');lcd_char('E');lcd_char('D');
   lcd_char(' ');lcd_char('O');lcd_char('F');lcd_char('F');
}
   }
}

void show_sub_item(int8 idx) {
   int8 nlen, olen, total, pad_l, pad_r;
   lcd_clear();

   // Fila 0: nombre del menú padre centrado
   nlen  = menu_name_len(menu_idx);
   pad_l = (16 - nlen) / 2;
   pad_r = 16 - nlen - pad_l;
   lcd_goto(0, 0);
   lcd_spaces(pad_l);
   lcd_print_menu_name(menu_idx);
   lcd_spaces(pad_r);

   // Fila 1: "< opcion >" centrado
   olen  = sub_option_len(menu_idx, idx);
   total = olen + 4;
   pad_l = (16 - total) / 2;
   if (pad_l < 0) pad_l = 0;
   pad_r = 16 - total - pad_l;
   if (pad_r < 0) pad_r = 0;
   lcd_goto(1, 0);
   lcd_spaces(pad_l);
   lcd_char('<'); lcd_char(' ');
   print_sub_option_text(menu_idx, idx);
   lcd_char(' '); lcd_char('>');
   lcd_spaces(pad_r);
}

// --- IDLE SCREEN -------------------------------------------------------------
void show_scroll(void) {
   int8 i, ch, vp;
   lcd_goto(0, 0);
   for (i = 0; i < 16; i++) {
      vp = scroll_pos + i;
      if (vp < 16 || vp >= (int8)(16 + scroll_len)) ch = ' ';
      else ch = scroll_text[vp - 16];
      lcd_char(ch);
   }
}

void lcd_print_idle_label(void) {
   lcd_goto(1, 0);
   lcd_spaces(2);
   lcd_char('P');lcd_char('r');lcd_char('e');lcd_char('s');lcd_char('s');
   lcd_char(' ');
   lcd_char('S');lcd_char('E');lcd_char('L');lcd_char('E');lcd_char('C');lcd_char('T');
   lcd_spaces(2);
}

// --- LED HELPERS -------------------------------------------------------------
// RA0-RA3: push-pull normales ? HIGH = encendido
// RA4    : open-drain         ? LED4_ON() = LOW = encendido
//                               LED4_OFF() = HIGH (alta impedancia) = apagado
// NUNCA se llama output_a() porque sobreescribe RA4 y lo fuerza a 0.
void all_leds_off(void) {
   output_low(LED1);
   output_low(LED2);
   output_low(LED3);
   output_low(LED4);
   output_low(LED5);
}

void set_single_led(int8 pos) {
   // Apagar todos primero (pin a pin, sin output_a)
   output_low(LED1);
   output_low(LED2);
   output_low(LED3);
   output_low(LED4);
   output_low(LED5);

   // Encender solo el seleccionado
   if      (pos == 0) output_high(LED1);
   else if (pos == 1) output_high(LED2);
   else if (pos == 2) output_high(LED3);
   else if (pos == 3) output_high(LED4);
   else if (pos == 4) output_high(LED5);
}

// --- APPLY SELECTION ---------------------------------------------------------
void apply_selection(void) {
   if (menu_idx == MENU_BOMBA) {
      if      (sub_idx == 0) pump_mode = 1;
      else if (sub_idx == 1) pump_mode = 2;
      else if (sub_idx == 2) pump_mode = 3;
      else                   pump_mode = 0;
      pump_cnt = 0;
   } else if (menu_idx == MENU_MOTOR) {
      if (sub_idx == 0) {
         motor_mode = 1;
         output_high(STEP_DIR);
      } else if (sub_idx == 1) {
         motor_mode = 2;
         output_low(STEP_DIR);
      } else {
         motor_mode = 0;
         output_low(STEP_DIR);
         output_low(STEP_PUL);
         step_state = 0;
      }
      motor_cnt = 0;
   } else if (menu_idx == MENU_LEDS) {
      if      (sub_idx == 0) { led_mode = 1; led_pos = 0; }
      else if (sub_idx == 1) { led_mode = 2; led_pos = 4; }
      else {
         led_mode = 0;
         all_leds_off();
      }
      led_cnt = 0;
   } else {
      if (sub_idx <= 10) {
      text_speed = sub_idx;
      }
      else if (sub_idx == 11) {
      output_high(LCD_BL);
      }
      else {
      output_low(LCD_BL);
      }
   }
}

// --- BUTTON DEBOUNCE ---------------------------------------------------------
int1 btn_pressed(int1 pin_val, int8 *last) {
   int1 result = 0;
   if (pin_val == 0 && *last == 1) {
      delay_ms(20 * MASTER_DELAY);
      if (pin_val == 0) result = 1;
   }
   *last = (int8)pin_val;
   return result;
}

// --- MAIN --------------------------------------------------------------------
void main(void) {

   // ADC completamente desactivado ? RA0-RA4 digitales puros
   setup_adc_ports(NO_ANALOGS);
   setup_adc(ADC_OFF);

   // Direcciones de puertos
   set_tris_a(0x00);   // RA0-RA7 salidas (RA4 sigue siendo salida open-drain)
   set_tris_b(0xFF);   // RB entradas (botones)
   set_tris_c(0x00);   // RC salidas
   set_tris_d(0x00);   // RD salidas (LCD)

   // Deshabilitar pull-ups internos de PORTB
   #byte OPTION_REG = 0x81
   OPTION_REG |= 0x80;

   // -- Inicialización de salidas PIN A PIN (NUNCA output_a() tras este punto) -
   // output_a(0x00) forzaría RA4=0 sobreescribiendo el TRIS y rompiendo el
   // control open-drain. Se inicializa cada pin individualmente.
   output_low(LED1);
   output_low(LED2);
   output_low(LED3);
   output_low(LED4);
   output_low(LED5);   
   output_low(PUMP);
   output_low(STEP_DIR);
   output_low(STEP_PUL);
   output_low(LCD_RS);
   output_low(LCD_E);
   output_low(LCD_BL);

   build_scroll_text();
   lcd_init();

   int8 last_left   = 1;
   int8 last_right  = 1;
   int8 last_select = 1;

   int16 pump_period_1 = 500;
   int16 pump_period_2 = 200;
   int16 pump_period_3 = 80;
   int16 motor_period  = 10;
   int16 led_period    = 200;

   sys_state   = STATE_IDLE;
   scroll_pos  = 0;
   scroll_cnt  = 0;
   timeout_cnt = 0;
   pump_cnt    = 0; pump_state = 0;
   motor_cnt   = 0; step_state = 0;
   led_cnt     = 0; led_pos    = 0;

   lcd_clear();
   show_scroll();
   lcd_print_idle_label();

   while (1) {

      delay_ms(1 * MASTER_DELAY);

      // -- Botones (activo bajo) ---------------------------------------------
      int1 bl = input(BTN_LEFT);
      int1 br = input(BTN_RIGHT);
      int1 bs = input(BTN_SELECT);

      int1 pressed_left   = btn_pressed(bl, &last_left);
      int1 pressed_right  = btn_pressed(br, &last_right);
      int1 pressed_select = btn_pressed(bs, &last_select);

      int1 any_btn = pressed_left | pressed_right | pressed_select;

      if (any_btn) timeout_cnt = 0;
      else         timeout_cnt++;

      // -- Timeout ? IDLE ----------------------------------------------------
      if (timeout_cnt >= (int16)(TIMEOUT_TICKS)) {
         sys_state   = STATE_IDLE;
         scroll_pos  = 0;
         scroll_cnt  = 0;
         timeout_cnt = 0;
         lcd_clear();
         show_scroll();
         lcd_print_idle_label();
      }

      // -- Máquina de estados ------------------------------------------------
      if (sys_state == STATE_IDLE) {

         if (pressed_select) {
            sys_state = STATE_MENU;
            menu_idx  = 0;
            show_main_menu();
         } else {
            // Scroll con velocidad real controlada por text_speed
            int16 scroll_period = get_scroll_period();
            scroll_cnt++;
            if (scroll_cnt >= scroll_period) {
               scroll_cnt = 0;
               scroll_pos++;
               if (scroll_pos >= (int8)(scroll_len + 16)) scroll_pos = 0;
               show_scroll();
            }
         }

      } else if (sys_state == STATE_MENU) {

         if (pressed_left) {
            if (menu_idx == 0) menu_idx = MENU_COUNT - 1;
            else               menu_idx--;
            show_main_menu();
         } else if (pressed_right) {
            menu_idx++;
            if (menu_idx >= MENU_COUNT) menu_idx = 0;
            show_main_menu();
         } else if (pressed_select) {
            sys_state = STATE_SUBMENU;
            sub_idx   = 0;
            show_sub_item(sub_idx);
         }

      } else { // STATE_SUBMENU

         int8 smax = sub_max();
         if (pressed_left) {
            if (sub_idx == 0) sub_idx = smax - 1;
            else              sub_idx--;
            show_sub_item(sub_idx);
         } else if (pressed_right) {
            sub_idx++;
            if (sub_idx >= smax) sub_idx = 0;
            show_sub_item(sub_idx);
         } else if (pressed_select) {
            apply_selection();
            sys_state = STATE_MENU;
            show_main_menu();
         }
      }

      // -- BOMBA (RC0) -------------------------------------------------------
      pump_cnt++;
      if (pump_mode != 0) {
         int16 pp;
         if      (pump_mode == 1) pp = pump_period_1;
         else if (pump_mode == 2) pp = pump_period_2;
         else                     pp = pump_period_3;
         if (pump_cnt >= pp) {
            pump_cnt   = 0;
            pump_state ^= 1;
            if (pump_state) output_high(PUMP); else output_low(PUMP);
         }
      } else {
         output_low(PUMP);
         pump_cnt   = 0;
         pump_state = 0;
      }

      // -- MOTOR (RC1=DIR, RC2=STEP) -----------------------------------------
      if (motor_mode != 0) {
         motor_cnt++;
         if (motor_cnt >= motor_period) {
            motor_cnt  = 0;
            step_state ^= 1;
            if (step_state) output_high(STEP_PUL);
            else            output_low(STEP_PUL);
         }
      } else {
         output_low(STEP_PUL);
         output_low(STEP_DIR);
         step_state = 0;
         motor_cnt  = 0;
      }

      // -- LEDs (RA0–RA4) ----------------------------------------------------
      if (led_mode != 0) {
         led_cnt++;
         if (led_cnt >= led_period) {
            led_cnt = 0;
            set_single_led(led_pos);
            if (led_mode == 1) {
               led_pos++;
               if (led_pos > 4) led_pos = 0;
            } else {
               if (led_pos == 0) led_pos = 4;
               else              led_pos--;
            }
         }
      }

   }
}
