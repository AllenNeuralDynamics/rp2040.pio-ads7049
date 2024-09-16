#include <pio_ads7049.h>

PIO_ADS7049::PIO_ADS7049(PIO pio,
                         uint8_t cs_pin, uint8_t sck_pin, uint8_t poci_pin,
                         int8_t existing_program_address)
:pio_{pio}, offset_{existing_program_address},
 samp_chan_{-1}, ctrl_chan_{-1}
{
    // load program if an existing program address is unspecified.
    if (existing_program_address < 0) // unspecified argument defaults to -1.
        offset_ = pio_add_program(pio_, &ads7049_program);
    sm_ = pio_claim_unused_sm(pio_, true);
    // Configure pio program.
    setup_pio_ads7049(pio_, sm_, offset_, cs_pin, sck_pin, poci_pin);
}

PIO_ADS7049::~PIO_ADS7049()
{
    reset();
    // FIXME: stop and unclaim PIO sm_; leave memory untouched since other
    //  programs can overwrite it.
}

void PIO_ADS7049::reset()
{
    // Kill stream-to-memory and interrupt triggering if configured.
    if (samp_chan_ != -1)
        dma_channel_cleanup(samp_chan_);
    if (ctrl_chan_ != -1)
        dma_channel_cleanup(ctrl_chan_);
    samp_chan_ = -1;
    ctrl_chan_ = -1;
}

void PIO_ADS7049::setup_dma_stream_to_memory(volatile uint16_t* starting_address,
                                             size_t sample_count)
{
    _setup_dma_stream_to_memory(starting_address, sample_count, false, 0,
                                nullptr);
}

void PIO_ADS7049::setup_dma_stream_to_memory_with_interrupt(
    volatile uint16_t* starting_address, size_t sample_count,
    int dma_irq_source, irq_handler_t handler_func)
{
    _setup_dma_stream_to_memory(starting_address, sample_count, true,
        dma_irq_source, handler_func);
}

void PIO_ADS7049::_setup_dma_stream_to_memory(
    volatile uint16_t* starting_address, size_t sample_count,
    bool trigger_interrupt, int dma_irq_source, irq_handler_t handler_func)
{
    // FIXME: check that dma_irq_source is either DMA_IRQ_0 or DMA_IRQ_1.
// Setup inspired by logic analyzer example:
// https://github.com/raspberrypi/pico-examples/blob/master/pio/logic_analyser/logic_analyser.c#L65

    // Clear input shift counter, and FIFO, to remove any leftover ISR content.
    pio_sm_clear_fifos(pio_, sm_);
    pio_sm_restart(pio_, sm_);

    // Save dma_irq_source so we can later clear the interrupt from the correct
    // source.
    dma_irq_source_ = dma_irq_source;

    // Get two open DMA channels.
    // samp_chan_ samples the adc, paced by DREQ_ADC and chained to ctrl_chan.
    // ctrl_chan_ reconfigures & retriggers samp_chan_ when samp_chan_ finishes.
    // samp_chan_ may also trigger an interrupt if configured to do so.
    samp_chan_ = dma_claim_unused_channel(true);
    ctrl_chan_ = dma_claim_unused_channel(true);
    //printf("Sample channel: %d\r\n", samp_chan_);
    //printf("Ctrl channel: %d\r\n", ctrl_chan_);
    dma_channel_config samp_conf = dma_channel_get_default_config(samp_chan_);
    dma_channel_config ctrl_conf = dma_channel_get_default_config(ctrl_chan_);

    // Setup Sample Channel.
    channel_config_set_transfer_data_size(&samp_conf, DMA_SIZE_16);
    channel_config_set_read_increment(&samp_conf, false); // read from pio rx fifo reg.
    channel_config_set_write_increment(&samp_conf, true);
    channel_config_set_irq_quiet(&samp_conf, !trigger_interrupt);
    // Pace data according to pio providing data.
    channel_config_set_dreq(&samp_conf, pio_get_dreq(pio_, sm_, false));
    channel_config_set_chain_to(&samp_conf, ctrl_chan_);
    channel_config_set_enable(&samp_conf, true);
    // Apply samp_chan_ configuration.
    dma_channel_configure(
        samp_chan_,      // Channel to be configured
        &samp_conf,
        nullptr,        // write (dst) address will be loaded by ctrl_chan_.
        &pio_->rxf[sm_],  // read (source) address. Does not change.
        sample_count,   // Number of word transfers i.e: count_of(adc_samples_dest).
        false           // Don't Start immediately.
    );

    // Enable interrupt request for the particular IRQ number if
    // trigger_interrupt is true.
    if (dma_irq_source == DMA_IRQ_0)
        dma_channel_set_irq0_enabled(samp_chan_, trigger_interrupt);
    else if (dma_irq_source == DMA_IRQ_1)
        dma_channel_set_irq1_enabled(samp_chan_, trigger_interrupt);
    // Connnect handler function and enable interrupt.
    if (trigger_interrupt && (handler_func != nullptr))
    {
        irq_set_exclusive_handler(dma_irq_source, handler_func);
        irq_set_enabled(dma_irq_source, true);
    }
    //printf("Configured DMA sample channel.\r\n");

    // Setup Reconfiguration Channel
    // This channel will write the starting address to the write address
    // "trigger" register, which will restart the DMA Sample Channel.
    data_ptr_[0] = {starting_address};
    //printf("data_ptr_[0] = 0x%x\r\n", data_ptr_[0]);
    channel_config_set_transfer_data_size(&ctrl_conf, DMA_SIZE_32);
    channel_config_set_read_increment(&ctrl_conf, false); // read a single uint32.
    channel_config_set_write_increment(&ctrl_conf, false);
    channel_config_set_irq_quiet(&ctrl_conf, true);
    channel_config_set_dreq(&ctrl_conf, DREQ_FORCE); // Go as fast as possible.
    channel_config_set_enable(&ctrl_conf, true);
    // Apply reconfig channel configuration.
    dma_channel_configure(
        ctrl_chan_,  // Channel to be configured
        &ctrl_conf,
        &dma_hw->ch[samp_chan_].al2_write_addr_trig, // dst address.
        data_ptr_,   // Read (src) address is a single array with the starting address.
        1,          // Number of word transfers.
        false       // Don't Start immediately.
    );
    dma_channel_start(ctrl_chan_);
    //printf("Configured DMA control channel.\r\n");
}

void PIO_ADS7049::start()
{
    // launch the PIO program.
    pio_ads7049_start(pio_, sm_);
}
