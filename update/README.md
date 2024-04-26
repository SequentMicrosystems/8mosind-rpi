# update

This is the 8-MOSFETS  Card firmware update tool.

## Usage

```bash
git clone https://github.com/SequentMicrosystems/8mosind-rpi.git
cd 8mosind-rpi/update/
./update 0
```

If you clone the repository already, skip the first step. 
The command will download the newest firmware version from our server and write it  to the board.
The stack level of the board must be provided as a parameter. 

## Warning
During firmware update we strongly recommend to disconnect all outputs from the board since they can change state unpredictably.
Please make shure that the I2C port is reseved to the update, meaning no program or script tries to access the I2C port during update.
