#include "main_loop.hh"

#include "error_handler.hh"
#include "usb_device.h"
#include "usbd_hid.h"

#include <blt/devices/nrf24/nrf24.hh>
#include <blt/gpio.hh>
#include <blt/messages.hh>
#include <blt/time.hh>
#include <spi/spi.hh>

SPI_HandleTypeDef* g_hspi = nullptr;
auto               fhSpi()
{
  return g_hspi;
}

const uint8_t nrf24_addresses[][6] = {"1Node", "2Node"};
uint8_t       rx_buffer[blt::message::motion::size];

void main_loop(SPI_HandleTypeDef* hspi)
{
  using namespace blt;
  using namespace gpio;
  using namespace time::literals;

  using led_red    = gpio::pin_out<PC, 6>;
  using led_blue   = gpio::pin_out<PC, 7>;
  using led_orange = gpio::pin_out<PC, 8>;
  using led_green  = gpio::pin_out<PC, 9>;
  using leds       = gpio::pin_out<PC, 6, 8, 9, 7>;

  using btn_pair = gpio::pin_in<PA, 0>;

  using csn = gpio::pin_out<PB, 2>;
  using ce  = gpio::pin_out<PB, 5>;

  g_hspi = hspi;

  time::init();
  leds::clear();

  nrf24::device<spi::device<fhSpi>, csn, ce> radio;

  led_green::set();

  if (!radio.init()) {
    error_handler();
  }

  if (!radio.test()) {
    error_handler();
  }

  radio.setChannel(40);
  radio.setRfPowerLevel<nrf24::Value::RfPowerMinimum>();
  radio.setDataRate<nrf24::Value::RfDatarate250kbps>();
  radio.setAutoAck(true);
  radio.setupAutoRetransmit<nrf24::Value::AutoRetransmitDelay1500us, 15>();
  radio.setAddressWidth(5);
  radio.openWritingPipe(nrf24_addresses[0]);
  radio.openReadingPipe(1, nrf24_addresses[1]);

  radio.startListening();
  radio.powerUp();

  led_green::clear();

  while (true) {
    led_blue::set();
    while (radio.available()) {
      led_blue::clear();
      led_orange::set();
      radio.read(rx_buffer, blt::message::motion::size);
    }

    led_orange::clear();
    USBD_HID_SendReport(&hUsbDeviceFS, rx_buffer, blt::message::motion::size);
  }

  error_handler();
}
