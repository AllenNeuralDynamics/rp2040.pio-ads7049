A CPU-free implementation for reading ADC values from the ADS7049.

This library uses DMA and a PIO program to stream analog values from an ADS7049 directly to memory at 2MHz (maximum data rate for this sensor).
The result is that the CPU can start this behavior in the background and simply check the memory location at any time for the latest measurement!

The target location can either be a variable or an array.
You can also optionally trigger an interrupt at the end of writing a sequence of values.

## Examples
To see this driver in action, have a look at the [examples](./examples) folder.
Additionally here are few projects that use this driver in the wild:
* [Harp.Device.Lickety-Split](https://github.com/AllenNeuralDynamics/harp.device.lickety-split)
* [Harp.Device.Treadmill](https://github.com/AllenNeuralDynamics/harp.device.treadmill)

## Requirements
This program assumes:
* 125MHz pio clock source

This program consumes:
* 2 available DMA channels
*  1 PIO state machine
* 18 state machine instructions

## FAQs
### Is there any risk of memory corruption?
Writes to a single variable from DMA to variable are atomic, so no mutex locking is required.
However, writing contininuously to an array is not double-buffered.
If you are writing a sequence of bytes to an array on a loop, you will need to make a local copy of it so that the memory is not overwritten on the next sample interval.
This can be achieved by triggering a `memcpy` on interrupt and working with the copy of the data.

### Does this program work on the ADS7029 and ADS7039?
As is, no without sacrificing data throughput
