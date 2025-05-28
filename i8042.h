#ifndef _LCOM_I8042_H_
#define _LCOM_I8042_H_

#include <lcom/lcf.h>

/** @defgroup i8042 i8042
 * @{
 *
 * Constants for programming the i8042 Keyboard Controller (KBC) and PS/2 Mouse.
 */

/* I/O port addresses */

#define KBC_CMD_REG  0x64 /**< @brief KBC Command Register */
#define KBC_STATUS_REG 0x64 /**< @brief KBC Status Register (read) */
#define KBC_IN_BUF   0x60 /**< @brief KBC Input Buffer (write) */
#define KBC_OUT_BUF  0x60 /**< @brief KBC Output Buffer (read) */

/* KBC IRQ lines */

#define KBD_IRQ 1  /**< @brief Keyboard IRQ line */
#define MOUSE_IRQ 12 /**< @brief Mouse IRQ line */

/* KBC Status Register Bits */

#define KBC_PARITY_ERROR BIT(7) /**< @brief Parity error - invalid data */
#define KBC_TIMEOUT_ERROR BIT(6) /**< @brief Timeout error - invalid data */
#define KBC_AUX BIT(5) /**< @brief Mouse data if set, keyboard data otherwise */
#define KBC_INH BIT(4) /**< @brief Inhibit flag: 0 if keyboard is inhibited */
#define KBC_A2 BIT(3) /**< @brief A2 input line: distinguishes data (0) from command (1) */
#define KBC_SYS BIT(2) /**< @brief System flag */
#define KBC_IBF BIT(1) /**< @brief Input Buffer Full - don't write if set */
#define KBC_OBF BIT(0) /**< @brief Output Buffer Full - data available for reading */

/* KBC Commands */

#define KBC_READ_CMD 0x20 /**< @brief Read Command Byte */
#define KBC_WRITE_CMD 0x60 /**< @brief Write Command Byte */
#define KBC_DISABLE_MOUSE 0xA7 /**< @brief Disable mouse */
#define KBC_ENABLE_MOUSE 0xA8 /**< @brief Enable mouse */
#define KBC_CHECK_MOUSE_IFC 0xA9 /**< @brief Check Mouse Interface */
#define KBC_WRITE_TO_MOUSE 0xD4 /**< @brief Write byte to mouse */

/* KBC Command Byte Bits */

#define CMD_INT    BIT(0) /**< @brief Enable interrupt on OBF from keyboard */
#define CMD_INT2   BIT(1) /**< @brief Enable interrupt on OBF from mouse */
#define CMD_DIS    BIT(4) /**< @brief Disable keyboard interface */
#define CMD_DIS2   BIT(5) /**< @brief Disable mouse interface */

/* Mouse Acknowledgement Bytes */

#define MOUSE_ACK  0xFA /**< @brief Acknowledgement byte */
#define MOUSE_NACK 0xFE /**< @brief Negative Acknowledgement (resend command) */
#define MOUSE_ERROR 0xFC /**< @brief Error in command */

/* Mouse Commands (to be sent after 0xD4) */

#define MOUSE_RESET             0xFF /**< @brief Reset */
#define MOUSE_RESEND            0xFE /**< @brief Resend (after error) */
#define MOUSE_SET_DEFAULTS      0xF6 /**< @brief Set Defaults */
#define MOUSE_DISABLE_DATA_REPORT 0xF5 /**< @brief Disable Data Reporting */
#define MOUSE_ENABLE_DATA_REPORT  0xF4 /**< @brief Enable Data Reporting */
#define MOUSE_SET_SAMPLE_RATE   0xF3 /**< @brief Set Sample Rate */
#define MOUSE_SET_REMOTE_MODE   0xF0 /**< @brief Set Remote Mode */
#define MOUSE_READ_DATA         0xEB /**< @brief Read Data (in Remote Mode) */
#define MOUSE_SET_STREAM_MODE   0xEA /**< @brief Set Stream Mode */
#define MOUSE_STATUS_REQUEST    0xE9 /**< @brief Request Status */
#define MOUSE_SET_RESOLUTION    0xE8 /**< @brief Set Resolution */
#define MOUSE_SET_SCALING_2_1   0xE7 /**< @brief Set Scaling 2:1 */
#define MOUSE_SET_SCALING_1_1   0xE6 /**< @brief Set Scaling 1:1 */

/**@}*/

#endif /* _LCOM_I8042_H_ */
