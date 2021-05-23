[![8mosind-rpi](readmeres/sequent.jpg)](https://www.sequentmicrosystems.com)

# 8mosind-rpi

[![8mosind-rpi](readmeres/8-MOS_V3.jpg)](https://www.sequentmicrosystems.com)

This is the command line and python functions to control the [Eight MOSFETS V3 8-Layer Stackable Card for Raspberry Pi](https://sequentmicrosystems.com/collections/industrial-automation/products/eight-mosfets-v3-br-8-layer-stackable-card-br-for-raspberry-pi).
For the older hardware version card use this [repository](https://github.com/SequentMicrosystems/8mosfet-rpi) 

First enable I2C communication:
```bash
~$ sudo raspi-config
```

## Usage

```bash
~$ git clone https://github.com/SequentMicrosystems/8mosind-rpi.git
~$ cd 8mosind-rpi/
~/8mosind-rpi$ sudo make install
```

Now you can access all the functions of the mosfets board through the command "8mosind". Use -h option for help:
```bash
~$ 8mosind -h
```

If you clone the repository any update can be made with the following commands:

```bash
~$ cd 8mosind-rpi/  
~/8mosind-rpi$ git pull
~/8mosind-rpi$ sudo make install
```  

### [Python library](https://github.com/SequentMicrosystems/8mosind-rpi/tree/master/python)
### [NodeRed](https://github.com/SequentMicrosystems/8mosind-rpi/tree/master/node-red-contrib-sm-8mosind)
