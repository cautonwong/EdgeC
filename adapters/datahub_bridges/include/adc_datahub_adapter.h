#ifndef ADC_DATAHUB_ADAPTER_H
#define ADC_DATAHUB_ADAPTER_H

#include "adc_bus_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

adc_bus_port_t adc_datahub_port(datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* ADC_DATAHUB_ADAPTER_H */
