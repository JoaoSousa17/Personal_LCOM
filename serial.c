#include "serial.h"
#include <lcom/lcf.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static int serial_fd = -1;

int serial_init() {
    /* Try to open serial port device */
    serial_fd = open("/dev/tty00", O_RDWR | O_NONBLOCK);
    
    if (serial_fd < 0) {
        printf("Error opening serial port /dev/tty00: %s\n", strerror(errno));
        return 1;
    }
    
    printf("Serial port /dev/tty00 opened successfully (fd=%d)\n", serial_fd);
    return 0;
}

int serial_send_char(char c) {
    if (serial_fd < 0) {
        printf("Serial port not initialized\n");
        return 1;
    }
    
    ssize_t result = write(serial_fd, &c, 1);
    if (result != 1) {
        printf("Error writing to serial port: %s\n", strerror(errno));
        return 1;
    }
    
    return 0;
}

int serial_send_string(const char *str) {
    if (serial_fd < 0) {
        printf("Serial port not initialized\n");
        return 1;
    }
    
    size_t len = strlen(str);
    ssize_t result = write(serial_fd, str, len);
    
    if (result != (ssize_t)len) {
        printf("Error writing string to serial port: %s\n", strerror(errno));
        return 1;
    }
    
    return 0;
}

bool serial_has_data() {
    if (serial_fd < 0) return false;
    
    /* Try to read one byte without blocking */
    char dummy;
    ssize_t result = read(serial_fd, &dummy, 1);
    
    if (result == 1) {
        /* Data was available, but we consumed it! 
           This is a simple implementation limitation */
        return true;
    }
    
    return false;
}

char serial_read_char() {
    if (serial_fd < 0) return 0;
    
    char c;
    ssize_t result = read(serial_fd, &c, 1);
    
    if (result == 1) {
        return c;
    }
    
    return 0; /* No data available */
}

int serial_read_line(char *buffer, int max_len, int timeout_ms) {
    if (serial_fd < 0 || buffer == NULL || max_len <= 0) return 1;
    
    int pos = 0;
    int timeout_ticks = timeout_ms / 50; /* Approximate 50ms per tick */
    
    while (pos < max_len - 1 && timeout_ticks > 0) {
        char c = serial_read_char();
        
        if (c != 0) {
            if (c == '\n' || c == '\r') {
                buffer[pos] = '\0';
                return 0; /* Line complete */
            }
            buffer[pos++] = c;
        } else {
            /* No data, wait a bit */
            usleep(50000); /* 50ms */
            timeout_ticks--;
        }
    }
    
    buffer[pos] = '\0';
    return (timeout_ticks <= 0) ? 1 : 0; /* Timeout or buffer full */
}

void serial_cleanup() {
    if (serial_fd >= 0) {
        close(serial_fd);
        printf("Serial port closed\n");
        serial_fd = -1;
    }
} 

