void setup() {


}

void loop() {


}

/*
 * Parse pattern:
 *   Data
 *     [0] - 0. Сhange mode, 1. Change Light Color, 2. Light Mode, 3. Brightness, 4. Empty Brightness, 5. Smooth(1,2 mode),
 *           6. Smooth(3..5 mode), 7. Strobe Smooth, 8. Strobe Period, 9. Rainbow Step(mode 2), a. Rainbow Step(mode 7.3),
 *           b. Color change speed c. Rainbow Period, d. Running Lights speed
 *     [1][2][3] - Data pack 1  (convert to Int [0,999])
 *     [4][5][6] - Data pack 2  (convert to Int [0,999])
 *     [7][8][9] - Data pack 3  (convert to Int [0,999])
 *   Empty symbol: #
 *   Additional information: 
 *     Ex. We receive "100" in Data pack 1, that means 100% of deffault, for SMOOTH 100% == 1.0; for STROBE_PERIOD == 1000 etc.
 *     1100#50100 - Change Light Color with LIGHT_COLOR = 255 * 1, LIGHT_SAT = 255 * 0.5, LIGHT_BRIGHT = 255 * 1;
 */

void readBT()
{
  char* buff;
  if (buff[0] != '#')
    switch (buff[0])
    {
      eeprom_timer = millis();
      eeprom_flag = true;
      case '0': // Сhange mode
        sub_mode_key = getInt(1, 3);
        if (sub_mode_key <= MODE_AMOUNT)
          this_mode = sub_mode_key;
        break;
      case '1': // Change Light Color
        LIGHT_COLOR = getInt(1, 3) * 255;
        LIGHT_SAT = getInt(4, 6) * 255;
        LIGHT_BRIGHT = getInt(7, 9) * 255;
        break;
      case '2': // Light Mode
        light_mode = getInt(1, 3) * 100;
        break;
      case '3': // Общая яркость
        subInt = getInt(1, 3);
        if ( subInt <= 1 ) BRIGHTNESS = subInt * 255;
        FastLED.setBrightness(BRIGHTNESS);
        break;
      case '4': // Яркость "не горящих"
        subInt = getInt(1, 3);
        if ( subInt <= 1 ) EMPTY_BRIGHT = subInt * 255;
        break;
      case '5': // Плавность(1,2)
        subInt = getInt(1, 3);
        if ( subInt < 0.05 )
          subInt = 0.05;
        SMOOTH = subInt;
        break;
      case '6': // Плавность(3..5)
        subInt = getInt(1, 3);
        if ( subInt < 0.05 )
          subInt = 0.05;
        SMOOTH_FREQ = subInt;
        break;
      case '7': // Плавность вспышек
        subInt = getInt(1, 3);
        if ( subInt < 0.05 )
          subInt = 0.05;
        STROBE_SMOOTH = subInt;
        break;
      case '8': // Частота вспышек
        subInt = getInt(1, 3) * 1000;
        if ( subInt < 20 )
          subInt = 20;
        STROBE_PERIOD = subInt;
        break;
      case '9': // Шаг радуги 1
        subInt = getInt(1, 3) * 20;
        if ( subInt < 0.5 )
          subInt = 0.5;
        RAINBOW_STEP = subInt;
        break;
      case 'a': // Шаг радуги 2
        subInt = getInt(1, 3) * 10;
        if ( subInt < 0.5 )
          subInt = 0.5;
        RAINBOW_STEP_2 = subInt;
        break;
      case 'b': // Скорость изменения цвета
        subInt = getInt(1, 3) * 255;
        if ( subInt < 10 )
          subInt = 10;
        COLOR_SPEED = subInt;
        break;
      case 'c': // Период радуги
        subInt = getInt(1, 3) * 20;
        if ( subInt < 1 )
          subInt = 1;
        RAINBOW_PERIOD = subInt;
        break;
      case 'd': // Скорость бегущих частот
        subInt = getInt(1, 3) * 255;
        if ( subInt < 10 )
          subInt = 10;
        RUNNING_SPEED = subInt;
        break;
      default:
        eeprom_flag = true;
        break;
    }
  buff[0] = '#';
}
 
double getInt(short startPos, const int endPos)
{
  int intBuffer[3] = {0, 0, 0};
  int count = 0;
  for (startPos; startPos <= endPos; startPos++)
  {
    if (buff[startPos] != '#')
      intBuffer[count] = buff[startPos] - '0';
    count++;
  }
  return intBuffer[0] + intBuffer[1] / 10 + intBuffer[2] / 100; // Возвращаем числовое значение
}
