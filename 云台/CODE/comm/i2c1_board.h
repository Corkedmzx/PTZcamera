#ifndef I2C1_BOARD_H
#define I2C1_BOARD_H

/*
 * 与常见 STM32F407 教程/外接排针一致：I2C1 常引出为 PB8=SCL、PB9=SDA。
 * 若你按「PB6=SCL、PB7=SDA」接线，请将下面宏改为 0。
 *
 * 改宏后须重新编译，并使杜邦线接法与宏一致。
 */
#ifndef I2C1_BOARD_PB8_PB9
#define I2C1_BOARD_PB8_PB9  1
#endif

#endif
