#include <pico/stdlib.h> // for uart printing
#include <pio_ads7049.h>
#include <cstdio> // for printf

// These can be any pins, and they need not be adjacent.
#define CS_PIN (18)
#define POCI_PIN (19)
#define SCK_PIN (20)

// Create device instance.
PIO_ADS7049 adc(pio0, CS_PIN, SCK_PIN, POCI_PIN);

// Create a placeholder for measurements.
uint16_t measurement;


// Core0 main.
int main()
{
    stdio_usb_init();
    // Configure DMA to continuously pull data from the PIO and write them here
    // at the maximum sample rate (2MHz).
    // Writes are atomic, so reading this value from the CPU will always return
    // the latest measurement.
    adc.setup_dma_stream_to_memory(&measurement, 1);
    adc.start();
    while(true)
    {
        printf("Raw ADC value: %d\r\n", measurement);
        sleep_ms(16);
    }
}
