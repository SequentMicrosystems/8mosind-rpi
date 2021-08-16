# node-red-contrib-sm-8mosind

This is the node-red node to control Sequent Microsystems [8-MOSFETS Stackable Card for Raspberry Pi](https://sequentmicrosystems.com/collections/industrial-automation/products/eight-mosfets-v3-br-8-layer-stackable-card-br-for-raspberry-pi).

## Manual Install

Clone or update the repository, follow the instrutions fron the [first page.](https://github.com/SequentMicrosystems/8mosind-rpi)

In your node-red user directory, tipicaly ~/.node-red,

```bash
~$ cd ~/.node-red
```

Run the following command:

```bash
~/.node-red$ npm install ~/8mosind-rpi/node-red-contrib-sm-8mosind
```

In order to see the node in the palette and use-it you need to restart node-red. If you run node-red as a service:
 ```bash
 ~$ node-red-stop
 ~$ node-red-start
 ```

## Usage

After install and restart the node-red you will see on the node palete, under Sequent Microsystems category the "8mosind" node.This node will turn on or off a mosfet. 
The card stack level and mosfet number can be set in the dialog screen or dinamicaly thru ``` msg.stack``` and ``` msg.mofet ```. The output of the mosfet can be set dynamically as a boolean using msg.payload.

## Important note

This node is using the I2C-bus package from @fivdi, you can visit his work on github [here](https://github.com/fivdi/i2c-bus). 
The inspiration for this node came from @nielsnl68 work with [node-red-contrib-i2c](https://github.com/nielsnl68/node-red-contrib-i2c).Thank both for the great job.
