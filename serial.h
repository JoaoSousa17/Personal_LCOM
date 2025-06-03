#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize serial port communication
 * @return 0 on success, non-zero on error
 */
int serial_init();

/**
 * @brief Send a single character through serial port
 * @param c Character to send
 * @return 0 on success, non-zero on error
 */
int serial_send_char(char c);

/**
 * @brief Send a string through serial port
 * @param str String to send
 * @return 0 on success, non-zero on error
 */
int serial_send_string(const char *str);

/**
 * @brief Check if there's data available to read
 * @return true if data available, false otherwise
 */
bool serial_has_data();

/**
 * @brief Read a single character from serial port (non-blocking)
 * @return Character read, or 0 if no data available
 */
char serial_read_char();

/**
 * @brief Read a line from serial port (blocking with timeout)
 * @param buffer Buffer to store the line
 * @param max_len Maximum length to read
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success, non-zero on timeout/error
 */
int serial_read_line(char *buffer, int max_len, int timeout_ms);

/**
 * @brief Cleanup serial port
 */
void serial_cleanup();

#endif /* _SERIAL_H_ */ 

