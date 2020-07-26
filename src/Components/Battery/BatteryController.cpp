#include <drivers/include/nrfx_saadc.h>
#include <hal/nrf_gpio.h>
#include <libraries/log/nrf_log.h>
#include "BatteryController.h"

/*
 * TODO: Save/load notable battery info from Flash
 */

using namespace Pinetime::Controllers;

void Battery::Init() {
  nrf_gpio_cfg_input(chargingPin, (nrf_gpio_pin_pull_t)GPIO_PIN_CNF_PULL_Pullup);
  nrf_gpio_cfg_input(powerPresentPin, (nrf_gpio_pin_pull_t)GPIO_PIN_CNF_PULL_Pullup);

  nrfx_saadc_config_t adcConfig = NRFX_SAADC_DEFAULT_CONFIG;
  nrfx_saadc_init(&adcConfig, SaadcEventHandler);
  nrf_saadc_channel_config_t adcChannelConfig = {
          .resistor_p = NRF_SAADC_RESISTOR_DISABLED,
          .resistor_n = NRF_SAADC_RESISTOR_DISABLED,
          .gain       = NRF_SAADC_GAIN1_5,
          .reference  = NRF_SAADC_REFERENCE_INTERNAL,
          .acq_time   = NRF_SAADC_ACQTIME_3US,
          .mode       = NRF_SAADC_MODE_SINGLE_ENDED,
          .burst      = NRF_SAADC_BURST_DISABLED,
          .pin_p      = batteryVoltageAdcInput,
          .pin_n      = NRF_SAADC_INPUT_DISABLED
  };
  nrfx_saadc_channel_init(0, &adcChannelConfig);
}

void Battery::Update() {
  isCharging = !nrf_gpio_pin_read(chargingPin);
  isPowerPresent = !nrf_gpio_pin_read(powerPresentPin);

  nrf_saadc_value_t value = 0;
  nrfx_saadc_sample_convert(0, &value);
  // see https://forum.pine64.org/showthread.php?tid=8147
  voltage = (value * 2.0f) / (1024 / 3.0f);

  UpdateState();
  CalcPercentRemaining();
  
  if(percentRemaining < percentLowPowerWarning) {
  	this->lowPowerMode = true;
  }
  else {
  	this->lowPowerMode = false;
  }
}

void Battery::SaadcEventHandler(nrfx_saadc_evt_t const * event) {

}

void Battery::UpdateState() {
    if(IsPowerPresent() && IsCharging()) {
        if(this->previousState == BatteryState::Charging) {
            NRF_LOG_INFO("CHARGING STARTED: " NRF_LOG_FLOAT_MARKER "V", NRF_LOG_FLOAT(voltage));
        }
        this->previousState = this->state;
        this->state = BatteryState::Charging;
    }
    else if(IsPowerPresent() && !IsCharging()) {
        if(previousState != BatteryState::ChargeComplete) {
            NRF_LOG_INFO("CHARGE COMPLETE: " NRF_LOG_FLOAT_MARKER "V", NRF_LOG_FLOAT(voltage));
        }

        this->previousState = this->state;
        state = BatteryState::ChargeComplete;
    }
    else if(!IsPowerPresent() && !IsCharging()) {
        // Save max voltage for reference later
        if(this->previousState == BatteryState::ChargeComplete) {
          fullVoltage = voltage;
        }

        // Store and Log Different Items
        if(this->previousState == BatteryState::Discharge) {
          if(voltage < emptyVoltage) {
            emptyVoltage = voltage;
            NRF_LOG_INFO("NEW LOW BATTERY: " NRF_LOG_FLOAT_MARKER "V", NRF_LOG_FLOAT(voltage));
          }
        }
        else if(this->previousState == BatteryState::Charging) {
          NRF_LOG_INFO("CHARGING STOPPED: " NRF_LOG_FLOAT_MARKER "V", NRF_LOG_FLOAT(voltage));
        }
        else if(this->previousState == BatteryState::ChargeComplete) {
          NRF_LOG_INFO("CHARGING STOPPED WITH FULL CHARGE: " NRF_LOG_FLOAT_MARKER "V", NRF_LOG_FLOAT(voltage));
        }

        this->previousState = this->state;
        this->state = BatteryState::Discharge;
    }
    else {
        this->previousState = this->state;
        this->state = BatteryState::BatteryStateUnknown;
    }
}

void Battery::CalcPercentRemaining() {

    if(this->state == BatteryState::Charging or this->state == BatteryState::ChargeComplete) {
        percentRemaining = ((voltage - emptyVoltage) * 100) * (fullVoltage + chargingVoltageOffset);
    }
    if(this->state == BatteryState::Discharge) {
        percentRemaining = ((voltage - emptyVoltage) * 100) * fullVoltage;
    }
}
