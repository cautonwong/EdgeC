#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "adc_bus_port.h"

#ifdef __cplusplus
extern "C" {
#endif

void adc_driver_handle_connection(int sock_fd, const adc_bus_port_t *bus);

#ifdef __cplusplus
}
#endif

#endif /* ADC_DRIVER_H */
