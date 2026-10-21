# update

This is the 8-MOSFETS  Card firmware update tool.

## Usage

```bash
git clone https://github.com/SequentMicrosystems/8mosind-rpi.git
cd 8mosind-rpi/update/
./update 0
```

If you have already cloned the repository, skip the first step. 
The command downloads the latest firmware version from our server and writes it to the board.
Provide the board stack level as a parameter. 
For the 64-bit Raspbian, use ``` ./update64 0 ```

## Warning
During firmware update, we strongly recommend disconnecting all outputs from the board since they can change state unpredictably.
Please make sure the I2C port is reserved for the update, meaning no program or script tries to access it during the update.
