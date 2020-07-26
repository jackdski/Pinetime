#pragma once
#include <drivers/include/nrfx_saadc.h>

#define SOC_LUT_SIZE    20

namespace Pinetime {
  namespace Controllers {
    class Battery {
      public:
        enum class BatteryState{ BatteryStateUnknown, Discharge, Charging, ChargeComplete };

        void Init();
        void Update();
        float PercentRemaining() const { return percentRemaining; }
        float Voltage() const { return voltage; }
        bool IsCharging() const { return isCharging; }
        bool IsPowerPresent() const { return isPowerPresent; }
        bool IsLowPowerMode() const { return lowPowerMode; }
        void UpdateState();
        void CalcPercentRemaining();
        BatteryState State() const { return state; }

      private:
        static constexpr uint32_t chargingPin = 12;
        static constexpr uint32_t powerPresentPin = 19;
        static constexpr nrf_saadc_input_t batteryVoltageAdcInput = NRF_SAADC_INPUT_AIN7;
        static void SaadcEventHandler(nrfx_saadc_evt_t const * p_event);
        float percentRemaining = 0.0f;
        float percentLowPowerWarning = 25.0f;
        float voltage = 0.0f;
        float fullVoltage = 3.90f;
        float emptyVoltage = 3.55f;
        float chargingVoltageOffset = 0.3f;
        bool isCharging = false;
        bool isPowerPresent = false;
        bool lowPowerMode = false;
        BatteryState state = BatteryState::BatteryStateUnknown;
        BatteryState previousState = BatteryState::BatteryStateUnknown;
    };
  }
}