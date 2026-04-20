# NeoXalleFirmware


Firmware for the NeoXalle reaction-training sport system.

[![Watch the video](https://img.youtube.com/vi/leAVq9ZJ1fE/maxresdefault.jpg)](https://youtu.be/leAVq9ZJ1fE)

### [Watch this video on YouTube](https://youtu.be/leAVq9ZJ1fE)



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
1. Waits for mobile app connection
2. Scans for slaves with name pattern `NeoXalle_Slave_*`
3. Connects to discovered slaves (max 2)
4. Routes commands from app to slaves
5. Forwards slave events back to app

### Slave Device
1. Waits for master connection
2. Monitors ADXL375 for taps (1.8g threshold)
3. Controls NeoPixel LEDs based on commands
4. Sends tap events with response time to master


## Related Repositories

- **Mobile App:** [NeoXalle](https://github.com/rekirxs/Neoxalle-APP)
- **HardWare** [NeoXalle](https://github.com/rekirxs/NeoXalle-Hardware)


