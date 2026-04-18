# NeoXalleFirmware


Firmware for the NeoXalle reaction-training sport system.


## Hardware Requirements

**HardWare** [NeoXalle](https://github.com/rekirxs/NeoXalle-Hardware)



## Files


  -Acelerometer_Code/MPUtest

  -Charge_Code/Charge_Code

  -Master_ESPNOW_TEST

  -NeoXalle_Master_Code/MASTER-CHG

  -NeoXalle_Slave_Code/Slave_Code


## How It Works

### Master Device
2. Waits for mobile app connection
3. Scans for slaves with name pattern `NeoXalle_Slave_*`
4. Connects to discovered slaves (max 2)
5. Routes commands from app to slaves
6. Forwards slave events back to app

### Slave Device
1. Waits for master connection
2. Monitors ADXL375 for taps (1.8g threshold)
3. Controls NeoPixel LEDs based on commands
4. Sends tap events with response time to master


## Related Repositories

- **Mobile App:** [NeoXalle](https://github.com/rekirxs/Neoxalle-APP)
- **HardWare** [NeoXalle](https://github.com/rekirxs/NeoXalle-Hardware)


