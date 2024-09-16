#ifndef PIO_ADS70X9_H
#define PIO_ADS70X9_H

#include <pico/stdlib.h>
#include <hardware/dma.h>
#include <hardware/pio.h>
#include <hardware/regs/dreq.h>
#include <stdio.h>
#include <stdint.h>
#include <pio_ads7049.pio.h> // auto-generated upon compilation.


/**
 * \brief class for acquiring data from ADS7029, ADS7039, and ADS7049
 *  SPI-based ADCs.
 */
class PIO_ADS7049
{
public:
/**
 * \brief constructor. Setup gpio pins, state machine.
 * \param existing_program_address if specifed as >=0, then the constructor will
 *  not add a pio program and instead use the existing one at the specified
 *  address.
 */
    PIO_ADS7049(PIO pio, uint8_t cs_pin, uint8_t sck_pin, uint8_t poci_pin,
                int8_t existing_program_address = -1);

    ~PIO_ADS7049();

/**
 * \brief release DMA if claimed.
 */
    void reset();

/**
 * \brief Configure continuous streaming of a specified number of values to a
 *  specified memory location at 2MHz.
*/
    // FIXME: make inline.
    void setup_dma_stream_to_memory(volatile uint16_t* starting_address,
                                    size_t sample_count);

/**
 * \brief Configure continuous streaming of a specified number of values to a
 *  specified memory location at 2MHz. Upon writing the specified number of
 *  values, trigger an interrupt to call the specified callback function.
*/
    // FIXME: make inline.
    void setup_dma_stream_to_memory_with_interrupt(
        volatile uint16_t* starting_address, size_t sample_count,
        int dma_irq_source, irq_handler_t handler_func);

/**
 * \brief Configure continuous streaming of a specified number of values to a
 *  specified memory location at 2MHz. Upon writing the specified number of
 *  values optionally trigger an interrupt.
 * \details Streaming occurs at the maximum data rate of the sensor (2MHz)
 *  and requires 2 DMA channels.
 * \param starting_address the starting address of the memory location to write
 *  new data to.
 * \param sample_count the number of samples to write to memory before looping
 *  back to the starting address.
 * \param trigger_interrupt if true, fire an interrupt upon writing
 *  `sample_count` samples to memory.
 * \param dma_irq_source DMA_IRQ_0 or DMA_IRQ_1
 * \param handler_func callback function. (This function must clear the
 *  corresponding interrupt source.
 * \note Although the AD7049 chips return 8, 10, or 12 bit data, DMA always
 *  reads 16-bit words from the PIO rx fifo.
 */
    void _setup_dma_stream_to_memory(volatile uint16_t* starting_address,
                                     size_t sample_count,
                                     bool trigger_interrupt,
                                     int dma_irq_source,
                                     irq_handler_t handler_func);

/**
 * \brief Get the program address (i.e: the offset) that the pio program was
 *  loaded at.
 */
    inline int8_t get_program_address()
    {return offset_;}

    inline void clear_interrupt()
    {
        if (dma_irq_source_ == DMA_IRQ_0)
            dma_hw->ints0 = 1u << samp_chan_;
        else if (dma_irq_source_ == DMA_IRQ_1)
            dma_hw->ints1 = 1u << samp_chan_;
    }

/**
 * \brief launch the pio program
 */
    void start();

private:
    int dma_irq_source_;
    int samp_chan_; // DMA channel used to collect samples and fire an interrupt
                    // if configured to do so. If it fires an interrupt,
                    // a DMA handler function needs to clear it.
    int ctrl_chan_; // DMA channel used to reconfigure and retrigger samp_chan_
                    // to re-run.

    PIO pio_;
    int8_t offset_;
    uint sm_;
    volatile uint16_t* data_ptr_[1]; // Data that the reconfiguration channel will write back
                                     // to the sample channel. In this case, just the
                                     // address of the location of the adc samples. This
                                     // value must exist with global scope since the DMA
                                     // reconfiguration channel will need to writes its value
                                     // back to the sample channel on regular intervals.


    uint32_t dma_data_chan_;
    uint32_t dma_ctrl_chan_;
};
#endif // PIO_ADS70X9_H
